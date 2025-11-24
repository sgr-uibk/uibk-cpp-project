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
		throw std::runtime_error("Failed to bind TCP port " + std::to_string(tcpPort) +
		                         " (port already in use or permission denied)");
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
	auto countValidPlayers = [this]() {
		int count = 0;
		for(const auto &p : m_slots)
			if(p.bValid)
				count++;
		return count;
	};

	// wait until host explicitly starts the game
	while(!m_gameStartRequested && !m_shutdownRequested)
	{
		int validPlayers = countValidPlayers();

		// keep processing until host requests start
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

	if(m_shutdownRequested)
	{
		SPDLOG_LOGGER_INFO(m_logger, "Server shutdown requested, exiting lobby loop");
		return;
	}

	int validPlayers = countValidPlayers();
	if(validPlayers >= 2 && m_cReady >= validPlayers)
	{
		SPDLOG_LOGGER_INFO(m_logger, "Host started game with {} ready players", validPlayers);
	}
	else
	{
		SPDLOG_LOGGER_WARN(m_logger, "Game start requested but conditions not met (players={}, ready={})", validPlayers,
		                   m_cReady);
	}
}

void LobbyServer::acceptNewClient()
{
	sf::TcpSocket newClientSock;
	if(m_listener.accept(newClientSock) != sf::Socket::Status::Done)
		return;
	assert(newClientSock.getRemoteAddress().has_value()); // Know exists from status done.

	// Reject connections if game is in progress
	if(m_gameInProgress)
	{
		SPDLOG_LOGGER_WARN(m_logger, "Game in progress, rejecting new connection");
		newClientSock.disconnect();
		return;
	}

	// Count current valid players
	int validPlayerCount = 0;
	for(const auto &slot : m_slots)
	{
		if(slot.bValid)
			validPlayerCount++;
	}

	if(validPlayerCount >= MAX_PLAYERS)
	{
		SPDLOG_LOGGER_WARN(m_logger, "Maximum lobby size reached ({}), rejecting player.", MAX_PLAYERS);
		newClientSock.disconnect();
		return;
	}

	// find an available slot (either reuse an invalid slot or assign new ID)
	uint32_t assignedId = 0;
	int slotIndex = -1;

	for(size_t i = 0; i < m_slots.size(); i++)
	{
		if(!m_slots[i].bValid)
		{
			assignedId = m_slots[i].id;
			slotIndex = static_cast<int>(i);
			SPDLOG_LOGGER_DEBUG(m_logger, "Reusing slot {} with id {}", slotIndex, assignedId);
			break;
		}
	}

	// If no invalid slot found, assign new ID
	if(assignedId == 0)
	{
		assignedId = m_nextId;
		m_nextId++;
	}

	sf::IpAddress const remoteAddr = newClientSock.getRemoteAddress().value();
	LobbyPlayer p{.id = assignedId, .udpAddr = remoteAddr, .tcpSocket = std::move(newClientSock)};
	// keep socket blocking until we receive JOIN_REQ
	p.tcpSocket.setBlocking(true);

	sf::Packet joinReqPkt;
	if(checkedReceive(p.tcpSocket, joinReqPkt) != sf::Socket::Status::Done)
	{
		p.tcpSocket.disconnect();
		return;
	}

	uint8_t type;
	joinReqPkt >> type;
	if(type != uint8_t(ReliablePktType::JOIN_REQ))
	{
		p.tcpSocket.disconnect();
		return;
	}

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

	// check for duplicate names
	std::string originalName = p.name;
	int suffix = 2;
	bool nameExists = true;
	while(nameExists)
	{
		nameExists = false;
		for(const auto &existingPlayer : m_slots)
		{
			if(existingPlayer.bValid && existingPlayer.name == p.name)
			{
				nameExists = true;
				p.name = originalName + " (" + std::to_string(suffix) + ")";
				suffix++;
				break;
			}
		}
	}

	if(p.name != originalName)
	{
		SPDLOG_LOGGER_INFO(m_logger, "Name '{}' already taken, assigned name '{}'", originalName, p.name);
	}

	sf::Packet joinAckPkt = createPkt(ReliablePktType::JOIN_ACK);
	joinAckPkt << p.id;
	joinAckPkt << p.name;
	SPDLOG_LOGGER_INFO(m_logger, "This client gets id: {} and name: '{}'", p.id, p.name);
	if(checkedSend(p.tcpSocket, joinAckPkt) != sf::Socket::Status::Done)
		SPDLOG_LOGGER_ERROR(m_logger, "Failed to send JOIN_ACK to {} (id {})", p.name, p.id);

	// set to non-blocking for normal lobby operation
	p.tcpSocket.setBlocking(false);

	// All good from server side. Make the client valid.
	p.bValid = true;
	m_multiSock.add(p.tcpSocket);
	SPDLOG_LOGGER_INFO(m_logger, "Player {} (id {}) joined lobby", p.name, p.id);

	// Either update existing slot or add new one
	if(slotIndex >= 0)
	{
		SPDLOG_LOGGER_DEBUG(m_logger, "Reusing slot {} for player {}", slotIndex, p.name);
		m_slots[slotIndex] = std::move(p);
	}
	else
	{
		m_slots.push_back(std::move(p));
	}

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

		// decrement ready count if player was ready
		if(p.bReady)
		{
			m_cReady--;
			SPDLOG_LOGGER_INFO(m_logger, "Player was ready, decrementing ready count to {}", m_cReady);
		}

		// host disconnects -> shut down the server
		if(p.id == 1)
		{
			SPDLOG_LOGGER_WARN(m_logger, "Host (player 1) disconnected, shutting down server");
			requestShutdown();
		}

		p.bValid = false;
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
		else if(type == static_cast<uint8_t>(ReliablePktType::LOBBY_UNREADY))
		{
			uint32_t clientId;
			rxPkt >> clientId;
			assert(clientId == p.id && p.bReady);
			p.bReady = false;
			m_cReady--;
			SPDLOG_LOGGER_INFO(m_logger, "Player {} (id {}) is now unready", p.name, p.id);
			broadcastLobbyUpdate();
		}
		else if(type == static_cast<uint8_t>(ReliablePktType::START_GAME_REQUEST))
		{
			if(p.id == 1)
			{
				SPDLOG_LOGGER_INFO(m_logger, "Host {} requested game start", p.name);
				m_gameStartRequested = true;
			}
			else
			{
				SPDLOG_LOGGER_WARN(m_logger, "Non-host player {} (id {}) tried to start game - ignoring", p.name, p.id);
			}
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

	std::vector<LobbyPlayer const *> validPlayers;
	for(auto const &c : m_slots)
	{
		if(c.bValid)
		{
			validPlayers.push_back(&c);

			// assign spawn point from shuffled list
			sf::Vector2f const spawn = spawns[validPlayers.size() - 1];
			sf::Angle const rot = sf::degrees(float(rng()) / float(rng.max() / 360));
			PlayerState ps(c.id, spawn);
			ps.m_name = c.name;
			ps.m_rot = rot;
			ps.m_kills = c.totalKills;
			ps.m_deaths = c.totalDeaths;
			worldState.setPlayer(ps);
		}
	}

	sf::Packet startPkt = createPkt(ReliablePktType::GAME_START);
	startPkt << validPlayers.size();

	for(auto const *validPlayer : validPlayers)
	{
		auto const &playerStates = worldState.getPlayers();
		for(auto const &ps : playerStates)
		{
			if(ps.m_id == validPlayer->id)
			{
				startPkt << ps.m_id;
				startPkt << validPlayer->name;
				startPkt << ps.getPosition();
				startPkt << ps.getRotation();
				SPDLOG_LOGGER_DEBUG(m_logger, "  Sending player {} ('{}') spawn: pos=({:.1f},{:.1f}), rot={:.1f}°",
				                    ps.m_id, validPlayer->name, ps.getPosition().x, ps.getPosition().y,
				                    ps.getRotation().asDegrees());
				break;
			}
		}
	}
	// Distribute spawn points with GAME_START pkt
	for(auto &p : m_slots)
	{
		if(!p.bValid)
			continue;
		if(checkedSend(p.tcpSocket, startPkt) != sf::Socket::Status::Done)
			SPDLOG_LOGGER_ERROR(m_logger, "Failed to send GAME_START to {} (id {})", p.name, p.id);
	}
	return std::move(worldState);
}

void LobbyServer::broadcastLobbyUpdate()
{
	uint32_t numPlayers = 0;
	for(const auto &p : m_slots)
	{
		if(p.bValid)
			numPlayers++;
	}

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

void LobbyServer::requestShutdown()
{
	if(m_shutdownRequested)
		return;

	SPDLOG_LOGGER_INFO(m_logger, "Server shutdown requested");
	m_shutdownRequested = true;

	broadcastServerShutdown();

	// close the listener immediately to release the port
	m_listener.close();
	SPDLOG_LOGGER_INFO(m_logger, "TCP listener closed, port released");
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

void LobbyServer::updatePlayerStats(const std::array<PlayerState, MAX_PLAYERS> &playerStates)
{
	for(auto &slot : m_slots)
	{
		if(!slot.bValid)
			continue;

		for(const auto &ps : playerStates)
		{
			if(ps.m_id == slot.id)
			{
				slot.totalKills = ps.getKills();
				slot.totalDeaths = ps.getDeaths();
				SPDLOG_LOGGER_INFO(m_logger, "Updated stats for {} (id {}): {} kills, {} deaths", slot.name, slot.id,
				                   slot.totalKills, slot.totalDeaths);
				break;
			}
		}
	}
}

void LobbyServer::resetLobbyState()
{
	// reset all ready flags after game ends
	for(auto &p : m_slots)
	{
		if(p.bValid)
		{
			p.bReady = false;
		}
	}
	m_cReady = 0;
	m_gameStartRequested = false;
	m_gameInProgress = false;
	SPDLOG_LOGGER_INFO(m_logger, "Lobby state reset: all players unmarked as ready");

	sf::sleep(sf::milliseconds(100));

	for(int i = 0; i < 3; ++i)
	{
		broadcastLobbyUpdate();
		if(i < 2)
			sf::sleep(sf::milliseconds(50));
	}

	SPDLOG_LOGGER_INFO(m_logger, "Broadcasted lobby state reset to all clients");
}
