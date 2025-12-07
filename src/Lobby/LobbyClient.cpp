#include "LobbyClient.h"
#include "../Utilities.h"

LobbyClient::LobbyClient(std::string const &name, Endpoint const lobbyServer)
	: m_clientId(0), m_name(name), m_bReady(false), m_loggerName(name)
{
	if(m_lobbySock.connect(lobbyServer.ip, lobbyServer.port) != sf::Socket::Status::Done)
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("Client"), "Failed to connect to lobby");
		throw std::runtime_error("Could not reach server.");
	}
	m_lobbySock.setBlocking(true);
	// Bind to the local game socket now, the server should know everything about the client as early as possible.
	bindGameSocket();
	SPDLOG_LOGGER_INFO(spdlog::get("Client"), "Starting Client. Server {}, lobby:{}, game:{}, client name {}",
	                   lobbyServer.ip.toString(), lobbyServer.port, m_gameSock.getLocalPort(), m_name);
}

LobbyClient::~LobbyClient()
{
	m_lobbySock.disconnect();
	m_gameSock.unbind();
	// drop the logger using the original name to allow recreating it later
	spdlog::drop(m_loggerName);
}

void LobbyClient::bindGameSocket()
{
	auto status = sf::Socket::Status::NotReady;
	for(uint16_t i = 1; i <= MAX_PLAYERS + 1; ++i)
	{
		uint16_t const gamePort = PORT_UDP + i;
		status = m_gameSock.bind(gamePort);
		if(status == sf::Socket::Status::Done)
		{
			SPDLOG_LOGGER_INFO(spdlog::get("Client"), "UDP Bind on port {} succeeded.", gamePort);
			break;
		}
		m_gameSock.unbind();
	}
	if(status != sf::Socket::Status::Done)
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("Client"), "Failed to bind to any UDP port [{}, {}]", PORT_UDP + 1,
		                    PORT_UDP + 1 + MAX_PLAYERS);
		throw std::runtime_error("Failed to bind to any available UDP port");
	}
	m_gameSock.setBlocking(false);
}

void LobbyClient::connect()
{
	sf::Packet joinPkt = createPkt(ReliablePktType::JOIN_REQ);
	joinPkt << PROTOCOL_VERSION;
	joinPkt << m_name << m_gameSock.getLocalPort();
	if(checkedSend(m_lobbySock, joinPkt) != sf::Socket::Status::Done)
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("Client"), "Failed to send JOIN_REQ");
		std::exit(1);
	}

	// Wait for JOIN_ACK with timeout to avoid indefinite blocking
	sf::Clock timeout;
	sf::Time const maxWaitTime = sf::seconds(5);

	while(timeout.getElapsedTime() < maxWaitTime)
	{
		sf::Packet joinAckPkt;
		sf::Socket::Status error = checkedReceive(m_lobbySock, joinAckPkt);

		if(error == sf::Socket::Status::Done)
		{
			uint8_t type;
			joinAckPkt >> type;
			if(type == uint8_t(ReliablePktType::JOIN_ACK))
			{
				std::string assignedName;
				joinAckPkt >> m_clientId >> assignedName;

				if(assignedName != m_name)
				{
					SPDLOG_LOGGER_INFO(spdlog::get("Client"), "Name changed from '{}' to '{}' due to duplicate", m_name,
					                   assignedName);
					m_name = assignedName;
				}

				SPDLOG_LOGGER_INFO(spdlog::get("Client"), "Joined lobby, got id {} with name '{}'", m_clientId, m_name);
				assert(m_clientId != 0);
				return;
			}
			SPDLOG_LOGGER_ERROR(spdlog::get("Client"), "Unexpected packet type {}", type);
			std::exit(1);
		}
		else if(error == sf::Socket::Status::NotReady)
		{
			// No data yet, sleep briefly and try again
			sf::sleep(sf::milliseconds(50));
		}
		else
		{
			SPDLOG_LOGGER_ERROR(spdlog::get("Client"), "Failed to receive JOIN_ACK: {}", int(error));
#ifdef SFML_SYSTEM_LINUX
			perror("Error from OS");
#endif
			std::exit(1);
		}
	}

	// Timeout occurred
	SPDLOG_LOGGER_ERROR(spdlog::get("Client"), "Timeout waiting for JOIN_ACK from server");
	std::exit(1);
}

void LobbyClient::sendReady()
{
	sf::Packet readyPkt = createPkt(ReliablePktType::LOBBY_READY);
	readyPkt << m_clientId;

	if(checkedSend(m_lobbySock, readyPkt) != sf::Socket::Status::Done)
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("Client"), "Failed to send LOBBY_READY");
	}
	SPDLOG_LOGGER_INFO(spdlog::get("Client"), "Sent ready packet, clientId {}", m_clientId);
	m_bReady = true;
}

LobbyEvent LobbyClient::pollLobbyUpdate()
{
	sf::Packet pkt;
	bool wasBlocking = m_lobbySock.isBlocking();
	m_lobbySock.setBlocking(false);

	sf::Socket::Status status = checkedReceive(m_lobbySock, pkt);

	m_lobbySock.setBlocking(wasBlocking);

	if(status != sf::Socket::Status::Done)
	{
		if(status == sf::Socket::Status::Disconnected)
		{
			SPDLOG_LOGGER_ERROR(spdlog::get("Client"), "Server disconnected");
			return LobbyEvent::SERVER_DISCONNECTED;
		}

		if(status != sf::Socket::Status::NotReady)
		{
			SPDLOG_LOGGER_WARN(spdlog::get("Client"), "pollLobbyUpdate: receive status = {}", static_cast<int>(status));
		}
		return LobbyEvent::NONE;
	}

	uint8_t type;
	pkt >> type;

	if(type == uint8_t(ReliablePktType::LOBBY_UPDATE))
	{
		uint32_t numPlayers;
		pkt >> numPlayers;

		m_lobbyPlayers.clear();
		for(uint32_t i = 0; i < numPlayers; ++i)
		{
			LobbyPlayerInfo playerInfo;
			pkt >> playerInfo.id >> playerInfo.name >> playerInfo.bReady;
			m_lobbyPlayers.push_back(playerInfo);

			SPDLOG_LOGGER_INFO(spdlog::get("Client"), "  Player {}: '{}' (ready={})", playerInfo.id, playerInfo.name,
			                   playerInfo.bReady);
		}

		SPDLOG_LOGGER_INFO(spdlog::get("Client"), "Received lobby update with {} players", numPlayers);
		return LobbyEvent::LOBBY_UPDATED;
	}
	if(type == uint8_t(ReliablePktType::GAME_START))
	{
		parseGameStartPacket(pkt);
		return LobbyEvent::GAME_STARTED;
	}

	SPDLOG_LOGGER_WARN(spdlog::get("Client"), "Received unexpected packet type {}", type);
	return LobbyEvent::NONE;
}

void LobbyClient::parseGameStartPacket(sf::Packet &pkt)
{
	for(auto &state : m_startData)
		state.m_id = 0;

	size_t numPlayers;
	pkt >> numPlayers;
	SPDLOG_LOGGER_INFO(spdlog::get("Client"), "Received GAME_START for {} players", numPlayers);

	for(unsigned i = 0; i < numPlayers; ++i)
	{
		sf::Vector2f pos;
		sf::Angle rot;
		pkt >> pos >> rot;

		m_startData[i] = PlayerState(i + 1, pos, rot);
		SPDLOG_LOGGER_INFO(spdlog::get("Client"), "Player {} ('{}') spawn point is ({},{}), direction angle = {}deg",
		                   i + 1, m_lobbyPlayers[i].name, pos.x, pos.y, rot.asDegrees());
	}
}