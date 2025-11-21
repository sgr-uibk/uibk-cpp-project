#include "LobbyServer.h"
#include <cassert>
#include <memory>
#include <random>
#include <spdlog/spdlog.h>

LobbyServer::LobbyServer(uint16_t tcpPort, const std::shared_ptr<spdlog::logger> &logger)
	: m_logger(logger), m_cReady(0)
{
	if(m_listener.listen(tcpPort) != sf::Socket::Status::Done)
	{
		SPDLOG_LOGGER_ERROR(m_logger, "Failed to listen on TCP {}", tcpPort);
		std::exit(1);
	}
	m_listener.setBlocking(false);
	m_multiSock.add(m_listener);
	m_slots.reserve(MAX_PLAYERS);
	SPDLOG_LOGGER_INFO(m_logger, "LobbyServer listening on TCP {}", tcpPort);
}

LobbyServer::~LobbyServer()
{
	for(auto &p : m_slots)
		p.tcpSocket.disconnect();
	m_multiSock.clear();
	m_listener.close();
}

void LobbyServer::lobbyLoop()
{
	while(m_cReady < MAX_PLAYERS)
	{
		if(!m_multiSock.wait(sf::milliseconds(250)))
		{
			sf::sleep(sf::milliseconds(100));
			continue;
		}

		// Got a message. Is it a new client ?
		if(m_multiSock.isReady(m_listener))
			acceptNewClient();
		// Do the known clients have something to say ?
		for(auto &p : m_slots)
		{
			if(!p.bValid)
				continue;
			handleClient(p);
		}
	}
}

void LobbyServer::acceptNewClient()
{
	sf::TcpSocket newClientSock;
	sf::Socket::Status st;
	if(st = m_listener.accept(newClientSock); st != sf::Socket::Status::Done)
	{
		SPDLOG_LOGGER_WARN(m_logger, "Accept failed: {}", int(st));
		return;
	}
	assert(newClientSock.getRemoteAddress().has_value()); // Know exists from status done.

	if(m_tentativeId > MAX_PLAYERS)
	{
		SPDLOG_LOGGER_WARN(m_logger, "Maximum lobby size reached, rejecting player.");
		// TODO Tell them they were rejected.
		newClientSock.disconnect();
		return;
	}

	sf::IpAddress const remoteAddr = newClientSock.getRemoteAddress().value();
	LobbyPlayer p{.id = m_tentativeId, .udpAddr = remoteAddr, .tcpSocket = std::move(newClientSock)};

	sf::Packet joinReqPkt;
	if(st = checkedReceive(p.tcpSocket, joinReqPkt); st != sf::Socket::Status::Done)
	{
		SPDLOG_LOGGER_WARN(m_logger, "Failed to receive a JOIN_REQUEST pkt {}", int(st));
	ABORT_CONN:
#ifdef SFML_SYSTEM_LINUX
		perror("Error from OS");
#endif
		p.tcpSocket.disconnect();
		return;
	}
	uint8_t type;
	joinReqPkt >> type;
	if(type != uint8_t(ReliablePktType::JOIN_REQ))
		goto ABORT_CONN;

	uint32_t protoVer;
	joinReqPkt >> protoVer;
	if(protoVer != PROTOCOL_VERSION)
	{
		SPDLOG_LOGGER_ERROR(m_logger, "Proto mismatch (client id {} has v{}, need v{})", p.id, protoVer,
		                    PROTOCOL_VERSION);
		p.tcpSocket.disconnect();
		return;
	}
	joinReqPkt >> p.name;
	joinReqPkt >> p.gamePort;
	SPDLOG_LOGGER_INFO(m_logger, "Client id {}, udpPort {} ", p.id, p.gamePort);
	assert(p.id && p.gamePort);

	sf::Packet joinAckPkt = createPkt(ReliablePktType::JOIN_ACK);
	joinAckPkt << p.id;
	SPDLOG_LOGGER_INFO(m_logger, "This client gets id: {}. ", p.id);
	if(checkedSend(p.tcpSocket, joinAckPkt) != sf::Socket::Status::Done)
		SPDLOG_LOGGER_ERROR(m_logger, "Failed to send JOIN_ACK to {} (id {})", p.name, p.id);

	// All good from server side. Make the client valid.
	p.bValid = true;
	m_tentativeId++;
	// Add the client to the pool, now that registration is over, continue non-blocking.
	p.tcpSocket.setBlocking(false);
	m_multiSock.add(p.tcpSocket);
	SPDLOG_LOGGER_INFO(m_logger, "Player {} (id {}) joined lobby", p.name, p.id);
	m_slots.push_back(std::move(p));
}

void LobbyServer::handleClient(LobbyPlayer &p)
{
	// TODO assuming tcpSocket is nonblocking, what do we get in case of nothing ?
	sf::Packet rxPkt;
	switch(auto const status = checkedReceive(p.tcpSocket, rxPkt))
	{
	case sf::Socket::Status::Disconnected:
		SPDLOG_LOGGER_INFO(m_logger, "Player {} (id {}) left lobby", p.name, p.id);
		m_multiSock.remove(p.tcpSocket);
		p.tcpSocket.disconnect();
		p.bValid = false;
		break;

	case sf::Socket::Status::Done: {
		uint8_t type;
		rxPkt >> type;
		if(type == static_cast<uint8_t>(ReliablePktType::LOBBY_READY))
		{
			uint32_t clientId;
			rxPkt >> clientId;
			assert(clientId == p.id && !p.bReady);
			p.bReady = true;
			m_cReady++;
			SPDLOG_LOGGER_INFO(m_logger, "Player {} (id {}) is ready", p.name, p.id);
		}
		break;
	}
	case sf::Socket::Status::NotReady:
		break;
	default:
		SPDLOG_LOGGER_WARN(m_logger, "Unhandled socket status {}", (int)status);
		break;
	}
}

void LobbyServer::startGame(WorldState &worldState)
{
	static std::mt19937_64 rng{}; // deterministic spawn points
	auto spawns = worldState.getMap().getSpawns();
	std::shuffle(spawns.begin(), spawns.end(), rng);
	for(auto const &c : m_slots)
	{
		if(!c.bValid)
			continue;

		// Every player gets a random spawn point and rotation
		sf::Angle const rot = sf::degrees(float(rng()) / float(rng.max() / 360));
		PlayerState ps(c.id, spawns[c.id - 1]);

		ps.m_rot = rot;
		worldState.setPlayer(ps);
	}

	sf::Packet startPkt = createPkt(ReliablePktType::GAME_START);
	auto const &playerStates = worldState.getPlayers();
	startPkt << playerStates.size();
	for(auto const &playerState : playerStates)
	{
		startPkt << playerState.getPosition();
		startPkt << playerState.getRotation();
	}
	// Distribute spawn points with GAME_START pkt
	for(auto &p : m_slots)
	{
		if(!p.bValid)
			continue;
		if(checkedSend(p.tcpSocket, startPkt) != sf::Socket::Status::Done)
			SPDLOG_LOGGER_ERROR(m_logger, "Failed to send GAME_START to {} (id {})", p.name, p.id);
	}
}

void LobbyServer::resetClientsReadiness()
{
	m_cReady = 0;
	for(auto &c : m_slots)
		c.bReady = false;
}

void LobbyServer::endGame(EntityId winner)
{
	resetClientsReadiness();
	sf::Packet gameEndPkt = createPkt(ReliablePktType::GAME_END);
	gameEndPkt << winner;
	for(auto &p : m_slots)
	{
		if(auto st = checkedSend(p.tcpSocket, gameEndPkt); st != sf::Socket::Status::Done)
			SPDLOG_LOGGER_ERROR(m_logger, "Failed to send GAME_END to {} (id {}): {}", p.name, p.id, (int)st);
	}
}
