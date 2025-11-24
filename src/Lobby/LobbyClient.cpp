#include "LobbyClient.h"
#include "../Utilities.h"

LobbyClient::LobbyClient(std::string const &name, Endpoint const lobbyServer)
	: m_clientId(0), m_name(name), m_bReady(false)
{
	m_logger = createConsoleAndFileLogger(name);
	if(m_lobbySock.connect(lobbyServer.ip, lobbyServer.port) != sf::Socket::Status::Done)
	{
		SPDLOG_LOGGER_ERROR(m_logger, "Failed to connect to lobby");
		std::exit(1);
	}
	m_lobbySock.setBlocking(true);
	// Bind to the local game socket now, the server should know everything about the client as early as possible.
	bindGameSocket();
	SPDLOG_LOGGER_INFO(m_logger, "Starting Client. Server {}, lobby:{}, game:{}, client name {}",
	                   lobbyServer.ip.toString(), lobbyServer.port, m_gameSock.getLocalPort(), m_name);
}

LobbyClient::~LobbyClient()
{
	m_lobbySock.disconnect();
	m_gameSock.unbind();
	// drop the logger to allow recreating it with the same name later
	spdlog::drop(m_name);
}

void LobbyClient::bindGameSocket()
{
	auto status = sf::Socket::Status::NotReady;
	for(uint16_t i = 0; i < MAX_PLAYERS + 1; ++i)
	{
		uint16_t const gamePort = PORT_UDP + i;
		status = m_gameSock.bind(gamePort);
		if(status == sf::Socket::Status::Done)
		{
			SPDLOG_LOGGER_INFO(m_logger, "UDP Bind on port {} succeeded.", gamePort);
			break;
		}
		m_gameSock.unbind();
	}
	if(status != sf::Socket::Status::Done)
	{
		SPDLOG_LOGGER_ERROR(m_logger, "Failed to bind to any UDP port [{}, {}]", m_gameSock.getLocalPort(),
		                    m_gameSock.getLocalPort() + MAX_PLAYERS);
		std::exit(1);
	}
	m_gameSock.setBlocking(false);
}

void LobbyClient::connect()
{
	sf::Packet joinPkt = createPkt(ReliablePktType::JOIN_REQ);
	joinPkt << PROTOCOL_VERSION;
	joinPkt << m_name;
	joinPkt << m_gameSock.getLocalPort();
	if(checkedSend(m_lobbySock, joinPkt) != sf::Socket::Status::Done)
	{
		SPDLOG_LOGGER_ERROR(m_logger, "Failed to send JOIN_REQ");
		std::exit(1);
	}

	sf::Packet joinAckPkt;
#ifdef SFML_SYSTEM_LINUX
	errno = 0;
#endif
	sf::Socket::Status error = checkedReceive(m_lobbySock, joinAckPkt);
	if(error == sf::Socket::Status::Done)
	{
		uint8_t type;
		joinAckPkt >> type;
		if(type == uint8_t(ReliablePktType::JOIN_ACK))
		{
			std::string assignedName;
			joinAckPkt >> m_clientId;
			joinAckPkt >> assignedName;

			if(assignedName != m_name)
			{
				SPDLOG_LOGGER_INFO(m_logger, "Name changed from '{}' to '{}' due to duplicate", m_name, assignedName);
				m_name = assignedName;
			}

			SPDLOG_LOGGER_INFO(m_logger, "Joined lobby, got id {} with name '{}'", m_clientId, m_name);
		}
		else
		{
			SPDLOG_LOGGER_ERROR(m_logger, "Unexpected packet type {}", type);
			std::exit(1);
		}
	}
	else
	{
		SPDLOG_LOGGER_ERROR(m_logger, "Failed to receive JOIN_ACK: {}", int(error));
#ifdef SFML_SYSTEM_LINUX
		perror("Error from OS");
#endif
		std::exit(1);
	}
	assert(m_clientId != 0);
}

void LobbyClient::sendReady()
{
	sf::Packet readyPkt = createPkt(ReliablePktType::LOBBY_READY);
	readyPkt << m_clientId;

	if(checkedSend(m_lobbySock, readyPkt) != sf::Socket::Status::Done)
	{
		SPDLOG_LOGGER_ERROR(m_logger, "Failed to send LOBBY_READY");
	}
	SPDLOG_LOGGER_INFO(m_logger, "Sent ready packet, clientId {}", m_clientId);
	m_bReady = true;
}

void LobbyClient::sendUnready()
{
	sf::Packet unreadyPkt = createPkt(ReliablePktType::LOBBY_UNREADY);
	unreadyPkt << m_clientId;

	if(checkedSend(m_lobbySock, unreadyPkt) != sf::Socket::Status::Done)
	{
		SPDLOG_LOGGER_ERROR(m_logger, "Failed to send LOBBY_UNREADY");
	}
	SPDLOG_LOGGER_INFO(m_logger, "Sent unready packet, clientId {}", m_clientId);
	m_bReady = false;
}

void LobbyClient::sendStartGame()
{
	sf::Packet startPkt = createPkt(ReliablePktType::START_GAME_REQUEST);

	if(checkedSend(m_lobbySock, startPkt) != sf::Socket::Status::Done)
	{
		SPDLOG_LOGGER_ERROR(m_logger, "Failed to send START_GAME_REQUEST");
	}
	SPDLOG_LOGGER_INFO(m_logger, "Host requested game start");
}

bool LobbyClient::pollLobbyUpdate()
{
	sf::Packet updatePkt;
	bool wasBlocking = m_lobbySock.isBlocking();
	m_lobbySock.setBlocking(false);

	sf::Socket::Status status = checkedReceive(m_lobbySock, updatePkt);

	// restore blocking state
	m_lobbySock.setBlocking(wasBlocking);

	if(status != sf::Socket::Status::Done)
	{
		if(status != sf::Socket::Status::NotReady)
		{
			SPDLOG_LOGGER_WARN(m_logger, "pollLobbyUpdate: receive status = {}", static_cast<int>(status));
		}
		return false;
	}

	uint8_t type;
	updatePkt >> type;

	if(type == uint8_t(ReliablePktType::SERVER_SHUTDOWN))
	{
		SPDLOG_LOGGER_WARN(m_logger, "Server is shutting down (host disconnected)");
		throw ServerShutdownException();
	}

	if(type != uint8_t(ReliablePktType::LOBBY_UPDATE))
	{
		SPDLOG_LOGGER_WARN(m_logger, "Expected LOBBY_UPDATE, got packet type {}", type);
		return false;
	}

	// parse lobby update
	uint32_t numPlayers;
	updatePkt >> numPlayers;

	m_lobbyPlayers.clear();
	for(uint32_t i = 0; i < numPlayers; ++i)
	{
		LobbyPlayerInfo playerInfo;
		updatePkt >> playerInfo.id >> playerInfo.name >> playerInfo.bReady;
		m_lobbyPlayers.push_back(playerInfo);
		SPDLOG_LOGGER_INFO(m_logger, "  Player {}: '{}' (ready={})", playerInfo.id, playerInfo.name, playerInfo.bReady);
	}

	SPDLOG_LOGGER_INFO(m_logger, "Received lobby update with {} players", numPlayers);
	return true;
}

std::array<PlayerState, MAX_PLAYERS> LobbyClient::parseGameStartPacket(sf::Packet &pkt)
{
	std::array<PlayerState, MAX_PLAYERS> states{};
	for(auto &state : states)
		state.m_id = 0;

	size_t numPlayers;
	pkt >> numPlayers;
	SPDLOG_LOGGER_INFO(m_logger, "Received GAME_START for {} players", numPlayers);

	for(unsigned i = 0; i < numPlayers; ++i)
	{
		uint32_t playerId;
		std::string playerName;
		sf::Vector2f pos;
		sf::Angle rot;
		uint32_t kills, deaths;
		pkt >> playerId >> playerName >> pos >> rot >> kills >> deaths;

		if(playerId > 0 && playerId <= MAX_PLAYERS)
		{
			states[playerId - 1] = PlayerState(playerId, pos, rot, GameConfig::Player::DEFAULT_MAX_HEALTH, playerName);
			states[playerId - 1].setKills(kills);
			states[playerId - 1].setDeaths(deaths);
			SPDLOG_LOGGER_INFO(m_logger, "Player {} ('{}') spawn point is ({},{}), direction angle = {}deg", playerId,
			                   playerName, pos.x, pos.y, rot.asDegrees());
		}
		else
		{
			SPDLOG_LOGGER_ERROR(m_logger, "Invalid player ID {} in GAME_START", playerId);
		}
	}
	return states;
}

std::optional<std::array<PlayerState, MAX_PLAYERS>> LobbyClient::checkForGameStart()
{
	sf::Packet startPkt;
	bool wasBlocking = m_lobbySock.isBlocking();
	m_lobbySock.setBlocking(false);

	sf::Socket::Status status = checkedReceive(m_lobbySock, startPkt);

	// restore blocking state
	m_lobbySock.setBlocking(wasBlocking);

	if(status != sf::Socket::Status::Done)
		return std::nullopt;

	uint8_t type;
	startPkt >> type;

	switch(type)
	{
	case uint8_t(ReliablePktType::SERVER_SHUTDOWN):
		SPDLOG_LOGGER_WARN(m_logger, "Server is shutting down (host disconnected)");
		throw ServerShutdownException();

	case uint8_t(ReliablePktType::LOBBY_UPDATE): {
		uint32_t numPlayers;
		startPkt >> numPlayers;
		m_lobbyPlayers.clear();
		for(uint32_t i = 0; i < numPlayers; ++i)
		{
			LobbyPlayerInfo playerInfo;
			startPkt >> playerInfo.id >> playerInfo.name >> playerInfo.bReady;
			m_lobbyPlayers.push_back(playerInfo);
		}
		SPDLOG_LOGGER_DEBUG(m_logger, "Received lobby update with {} players while waiting", numPlayers);
		return std::nullopt;
	}
	case uint8_t(ReliablePktType::GAME_START):
		return parseGameStartPacket(startPkt);

	default:
		SPDLOG_LOGGER_WARN(m_logger, "Unhandled reliable packet type: {}.", type);
		return std::nullopt;
	}
}

std::array<PlayerState, MAX_PLAYERS> LobbyClient::waitForGameStart()
{
	while(true)
	{
		sf::Packet startPkt;
		if(checkedReceive(m_lobbySock, startPkt) != sf::Socket::Status::Done)
		{
			SPDLOG_LOGGER_ERROR(m_logger, "Failed to receive GAME_START");
			std::exit(1);
		}

		uint8_t type;
		startPkt >> type;
		switch(type)
		{
		case uint8_t(ReliablePktType::SERVER_SHUTDOWN):
			SPDLOG_LOGGER_WARN(m_logger, "Server is shutting down (host disconnected)");
			throw ServerShutdownException();

		case uint8_t(ReliablePktType::LOBBY_UPDATE): {
			uint32_t numPlayers;
			startPkt >> numPlayers;
			m_lobbyPlayers.clear();
			for(uint32_t i = 0; i < numPlayers; ++i)
			{
				LobbyPlayerInfo playerInfo;
				startPkt >> playerInfo.id >> playerInfo.name >> playerInfo.bReady;
				m_lobbyPlayers.push_back(playerInfo);
			}
			SPDLOG_LOGGER_DEBUG(m_logger, "Received lobby update with {} players while waiting", numPlayers);
			break;
		}
		case uint8_t(ReliablePktType::GAME_START):
			return parseGameStartPacket(startPkt);

		default:
			SPDLOG_LOGGER_WARN(m_logger, "Unhandled reliable packet type: {}.", type);
		}
		sf::sleep(sf::milliseconds(100));
	}
}
