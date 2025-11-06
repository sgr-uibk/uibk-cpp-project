#include "LobbyClient.h"
#include "../Utilities.h"

LobbyClient::LobbyClient(std::string const &name, Endpoint const lobbyServer)
	: m_clientId(0),
	  m_name(name),
	  m_bReady(false)
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
		SPDLOG_LOGGER_ERROR(m_logger, "Failed to bind to any UDP port [{}, {}]",
		                    m_gameSock.getLocalPort(), m_gameSock.getLocalPort() + MAX_PLAYERS);
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
			joinAckPkt >> m_clientId;
			SPDLOG_LOGGER_INFO(m_logger, "Joined lobby, got id {}", m_clientId);
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

std::array<PlayerState, MAX_PLAYERS> LobbyClient::waitForGameStart()
{
	while(true)
	{
		sf::Packet startPkt;
		auto const st = checkedReceive(m_lobbySock, startPkt);
		switch(st)
		{
		case sf::Socket::Status::Done:
			break;
		case sf::Socket::Status::NotReady:
			continue;
		default:
			SPDLOG_LOGGER_ERROR(m_logger, "Failed to receive GAME_START: {}", (int)st);
			std::exit(1);
		}

		uint8_t type;
		startPkt >> type;
		switch(type)
		{
		case uint8_t(ReliablePktType::GAME_START): {
			std::array<PlayerState, MAX_PLAYERS> states;
			size_t numPlayers;
			startPkt >> numPlayers;
			SPDLOG_LOGGER_INFO(m_logger, "Received GAME_START for {} players", numPlayers);
			for(unsigned i = 0; i < numPlayers; ++i)
			{
				sf::Vector2f pos;
				startPkt >> pos;
				sf::Angle rot;
				startPkt >> rot;
				states[i] = PlayerState(i + 1, pos, rot);
				SPDLOG_LOGGER_INFO(m_logger, "My spawn point is ({},{}), direction angle = {}deg",
				                   pos.x, pos.y, rot.asDegrees());
			}
			return states;
		}
		default:
			SPDLOG_LOGGER_WARN(m_logger, "Unhandled reliable packet type: {}.", type);
		}
		sf::sleep(sf::milliseconds(100));
	}
}
