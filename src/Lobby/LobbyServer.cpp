#include "LobbyServer.h"
#include <cassert>
#include <memory>
#include <random>
#include <spdlog/spdlog.h>

LobbyServer::LobbyServer(uint16_t tcpPort)
{
	if(m_listener.listen(tcpPort) != sf::Socket::Status::Done)
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("Server"), "Failed to listen on TCP {}", tcpPort);
		std::exit(1);
	}
	m_listener.setBlocking(false);
	m_multiSock.add(m_listener);
	m_slots.reserve(MAX_PLAYERS);
	SPDLOG_LOGGER_INFO(spdlog::get("Server"), "LobbyServer listening on TCP {}", tcpPort);
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
			SPDLOG_LOGGER_INFO(spdlog::get("Server"), "Name already taken, assigned name '{}'", name);
		}
	}
}

void LobbyServer::acceptNewClient()
{
	sf::TcpSocket newClientSock;
	if(m_listener.accept(newClientSock) != sf::Socket::Status::Done)
		return;
	assert(newClientSock.getRemoteAddress().has_value()); // Know exists from status done.

	// Count current valid players
	int validPlayerCount = 0;
	for(const auto &slot : m_slots)
	{
		if(slot.bValid)
			validPlayerCount++;
	}

	if(validPlayerCount >= MAX_PLAYERS)
	{
		SPDLOG_LOGGER_WARN(spdlog::get("Server"), "Maximum lobby size reached ({}), rejecting player.", MAX_PLAYERS);
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
		SPDLOG_LOGGER_ERROR(spdlog::get("Server"), "Proto mismatch (client id {} has v{}, need v{})", p.id, protoVer,
		                    PROTOCOL_VERSION);
		p.tcpSocket.disconnect();
		return;
	}
	joinReqPkt >> p.name >> p.gamePort;
	SPDLOG_LOGGER_INFO(spdlog::get("Server"), "Client id {}, udpPort {}, requested name '{}' ", p.id, p.gamePort,
	                   p.name);
	assert(p.id && p.gamePort);

	deduplicatePlayerName(p.name);
	sf::Packet joinAckPkt = createPkt(ReliablePktType::JOIN_ACK);
	joinAckPkt << p.id << p.name;
	SPDLOG_LOGGER_INFO(spdlog::get("Server"), "This client gets id: {} and name: '{}'", p.id, p.name);
	if(checkedSend(p.tcpSocket, joinAckPkt) != sf::Socket::Status::Done)
		SPDLOG_LOGGER_ERROR(spdlog::get("Server"), "Failed to send JOIN_ACK to {} (id {})", p.name, p.id);

	// All good from server side. Make the client valid.
	p.bValid = true;
	// Add the client to the pool, now that registration is over, continue non-blocking.
	p.tcpSocket.setBlocking(false);
	m_multiSock.add(p.tcpSocket);
	SPDLOG_LOGGER_INFO(spdlog::get("Server"), "Player {} (id {}) joined lobby", p.name, p.id);

	// Either update existing slot or add new one
	if(slotIndex >= 0)
	{
		SPDLOG_LOGGER_DEBUG(spdlog::get("Server"), "Reusing slot {} for player {}", slotIndex, p.name);
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
		SPDLOG_LOGGER_INFO(spdlog::get("Server"), "Player {} (id {}) left lobby", p.name, p.id);
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
			SPDLOG_LOGGER_INFO(spdlog::get("Server"), "Player {} (id {}) is ready", p.name, p.id);
			broadcastLobbyUpdate();
		}
		break;
	}
	case sf::Socket::Status::NotReady:
		break;
	default:
		SPDLOG_LOGGER_WARN(spdlog::get("Server"), "Unhandled socket status {}", (int)status);
		break;
	}
}

void LobbyServer::startGame(WorldState &worldState)
{
	static std::mt19937_64 rng{}; // deterministic spawn points
	auto spawns = worldState.getMap().getSpawns();
	std::shuffle(spawns.begin(), spawns.end(), rng);

	size_t spawnIndex = 0;
	for(auto const &c : m_slots)
	{
		if(!c.bValid)
			continue;

		// assign spawn point from shuffled list
		sf::Vector2f const spawn = spawns[spawnIndex];
		sf::Angle const rot = sf::degrees(360.f * rng() / rng.max());
		PlayerState ps(c.id, spawn);
		ps.m_name = c.name;
		ps.m_rot = rot;
		ps.m_kills = c.totalKills;
		ps.m_deaths = c.totalDeaths;
		worldState.setPlayer(ps);
		spawnIndex++;
	}

	sf::Packet startPkt = createPkt(ReliablePktType::GAME_START);
	auto const &playerStates = worldState.getPlayers();
	startPkt << playerStates.size();
	for(auto const &playerState : playerStates)
	{
		startPkt << playerState.getPosition() << playerState.getRotation();
	}
	// Distribute spawn points with GAME_START pkt
	for(auto &p : m_slots)
	{
		if(!p.bValid)
			continue;
		if(checkedSend(p.tcpSocket, startPkt) != sf::Socket::Status::Done)
			SPDLOG_LOGGER_ERROR(spdlog::get("Server"), "Failed to send GAME_START to {} (id {})", p.name, p.id);
	}
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
			SPDLOG_LOGGER_ERROR(spdlog::get("Server"), "Failed to send GAME_END to {} (id {}): {}", p.name, p.id,
			                    (int)st);
	}
}

void LobbyServer::broadcastLobbyUpdate()
{
	uint32_t numPlayers = 0;
	for(auto const &p : m_slots)
		numPlayers += p.bValid;

	sf::Packet updatePkt = createPkt(ReliablePktType::LOBBY_UPDATE);
	updatePkt << numPlayers;

	for(auto const &p : m_slots)
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
				SPDLOG_LOGGER_WARN(spdlog::get("Server"), "Failed to send LOBBY_UPDATE to {} (id {})", p.name, p.id);
			}
		}
	}

	SPDLOG_LOGGER_DEBUG(spdlog::get("Server"), "Broadcasted lobby update to {} players", numPlayers);
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
				SPDLOG_LOGGER_INFO(spdlog::get("Server"), "Updated stats for {} (id {}): {} kills, {} deaths", slot.name, slot.id,
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
	SPDLOG_LOGGER_INFO(spdlog::get("Server"), "Lobby state reset: all players unmarked as ready");

	sf::sleep(sf::milliseconds(100));

	for(int i = 0; i < 3; ++i)
	{
		broadcastLobbyUpdate();
		if(i < 2)
			sf::sleep(sf::milliseconds(50));
	}

	SPDLOG_LOGGER_INFO(spdlog::get("Server"), "Broadcasted lobby state reset to all clients");
}
