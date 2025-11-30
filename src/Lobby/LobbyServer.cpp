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

void LobbyServer::deduplicatePlayerName(std::string &name) const
{
	static int suffix = 0;
	bool nameExists = false;
	for(size_t i = 0; !nameExists && i < m_slots.size(); ++i)
	{
		auto const &player = m_slots[i];
		nameExists = player.bValid && player.name == name;
		if(nameExists)
		{
			name.append(" (" + std::to_string(++suffix) + ")");
			SPDLOG_LOGGER_INFO(m_logger, "Name already taken, assigned name '{}'", name);
		}
	}
}

void LobbyServer::acceptNewClient()
{
	sf::TcpSocket newClientSock;
	if(m_listener.accept(newClientSock) != sf::Socket::Status::Done)
		return;
	assert(newClientSock.getRemoteAddress().has_value()); // Know exists from status done.

	if(m_nextId > MAX_PLAYERS)
	{
		SPDLOG_LOGGER_WARN(m_logger, "Maximum lobby size reached, rejecting player.");
		// TODO Tell them they were rejected.
		newClientSock.disconnect();
		return;
	}

	sf::IpAddress const remoteAddr = newClientSock.getRemoteAddress().value();
	LobbyPlayer p{.id = m_nextId, .udpAddr = remoteAddr, .tcpSocket = std::move(newClientSock)};

	sf::Packet joinReqPkt;
	if(checkedReceive(p.tcpSocket, joinReqPkt) != sf::Socket::Status::Done)
	{
	ABORT_CONN:
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
	SPDLOG_LOGGER_INFO(m_logger, "Client id {}, udpPort {}, requested name '{}' ", p.id, p.gamePort, p.name);
	assert(p.id && p.gamePort);

	deduplicatePlayerName(p.name);
	sf::Packet joinAckPkt = createPkt(ReliablePktType::JOIN_ACK);
	joinAckPkt << p.id << p.name;
	SPDLOG_LOGGER_INFO(m_logger, "This client gets id: {} and name: '{}'", p.id, p.name);
	if(checkedSend(p.tcpSocket, joinAckPkt) != sf::Socket::Status::Done)
		SPDLOG_LOGGER_ERROR(m_logger, "Failed to send JOIN_ACK to {} (id {})", p.name, p.id);

	// All good from server side. Make the client valid.
	p.bValid = true;
	m_nextId++;
	// Add the client to the pool, now that registration is over, continue non-blocking.
	p.tcpSocket.setBlocking(false);
	m_multiSock.add(p.tcpSocket);
	SPDLOG_LOGGER_INFO(m_logger, "Player {} (id {}) joined lobby", p.name, p.id);
	m_slots.push_back(std::move(p));
	broadcastLobbyUpdate();
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
		m_cReady -= p.bReady;
		p.bValid = p.bReady = false;
		broadcastLobbyUpdate();
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
			broadcastLobbyUpdate();
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

WorldState LobbyServer::startGame(WorldState &worldState)
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
	return worldState;
}

void LobbyServer::endGame(EntityId winner)
{
	m_cReady = 0;
	for(auto &c : m_slots)
		c.bReady = false;

	sf::Packet gameEndPkt = createPkt(ReliablePktType::GAME_END);
	gameEndPkt << winner;
	for(auto &p : m_slots)
	{
		if(auto st = checkedSend(p.tcpSocket, gameEndPkt); st != sf::Socket::Status::Done)
			SPDLOG_LOGGER_ERROR(m_logger, "Failed to send GAME_END to {} (id {}): {}", p.name, p.id, (int)st);
	}
}

void LobbyServer::broadcastLobbyUpdate()
{
	uint32_t numPlayers = 0;
	for(const auto &p : m_slots)
		numPlayers += p.bValid;

	sf::Packet updatePkt = createPkt(ReliablePktType::LOBBY_UPDATE);
	updatePkt << numPlayers;

	for(const auto &p : m_slots)
	{
		if(p.bValid)
		{
			updatePkt << p.id << p.name << p.bReady;
		}
	}

	for(auto &p : m_slots)
	{
		if(p.bValid)
		{
			if(checkedSend(p.tcpSocket, updatePkt) != sf::Socket::Status::Done)
			{
				SPDLOG_LOGGER_WARN(m_logger, "Failed to send LOBBY_UPDATE to {} (id {})", p.name, p.id);
			}
		}
	}

	SPDLOG_LOGGER_DEBUG(m_logger, "Broadcasted lobby update to {} players", numPlayers);
}

void LobbyServer::broadcastServerShutdown()
{
	SPDLOG_LOGGER_INFO(m_logger, "Broadcasting SERVER_SHUTDOWN to all clients");
	sf::Packet shutdownPkt = createPkt(ReliablePktType::SERVER_SHUTDOWN);

	for(auto &p : m_slots)
	{
		if(p.bValid)
		{
			if(checkedSend(p.tcpSocket, shutdownPkt) != sf::Socket::Status::Done)
			{
				SPDLOG_LOGGER_WARN(m_logger, "Failed to send SERVER_SHUTDOWN to {} (id {})", p.name, p.id);
			}
			else
			{
				SPDLOG_LOGGER_INFO(m_logger, "Sent SERVER_SHUTDOWN to {} (id {})", p.name, p.id);
			}
		}
	}
}
