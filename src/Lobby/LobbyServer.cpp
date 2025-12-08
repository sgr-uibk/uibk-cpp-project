#include "LobbyServer.h"
#include <cassert>
#include <memory>
#include <random>
#include <spdlog/spdlog.h>
#include <stdexcept>

LobbyServer::LobbyServer(uint16_t tcpPort)
{
	if(m_listener.listen(tcpPort) != sf::Socket::Status::Done)
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("Server"), "Failed to listen on TCP {}", tcpPort);
		throw std::runtime_error("Port " + std::to_string(tcpPort) + " is already in use.");
	}
	m_listener.setBlocking(false);
	m_multiSock.add(m_listener);
	m_slots.reserve(MAX_PLAYERS);
	m_running = true;
	SPDLOG_LOGGER_INFO(spdlog::get("Server"), "LobbyServer listening on TCP {}", tcpPort);
}

LobbyServer::~LobbyServer()
{
	stop();
	m_listener.close();
	for(auto &p : m_slots)
		p.tcpSocket.disconnect();
	m_multiSock.clear();
}

void LobbyServer::lobbyLoop()
{
	SPDLOG_LOGGER_INFO(spdlog::get("Server"), "Lobby Loop Started. Waiting for {} players...", MAX_PLAYERS);

	while(m_running)
	{
		size_t connectedCount = 0;
		size_t readyCount = 0;

		for(auto const &p : m_slots)
		{
			if(p.bValid)
			{
				connectedCount++;
				if(p.bReady)
					readyCount++;
			}
		}

		if(connectedCount == MAX_PLAYERS && readyCount == MAX_PLAYERS)
		{
			SPDLOG_LOGGER_INFO(spdlog::get("Server"), "Lobby Full and Ready! Starting Game...");
			return; // Exit loop, which triggers startGame() in the main thread
		}

		if(m_multiSock.wait(sf::milliseconds(100)))
		{
			if(m_multiSock.isReady(m_listener))
				acceptNewClient();
			// Do the known clients have something to say ?
			for(auto &p : m_slots)
			{
				if(p.bValid && m_multiSock.isReady(p.tcpSocket))
					handleClient(p);
			}
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

	// Count current valid players
	int validPlayerCount = 0;
	for(auto const &slot : m_slots)
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

	newClientSock.setBlocking(true);

	sf::Packet joinReqPkt;
	// Use a small timeout for the handshake so we don't freeze the server
	sf::SocketSelector selector;
	selector.add(newClientSock);
	if(!selector.wait(sf::milliseconds(500)))
	{
		newClientSock.disconnect();
		return;
	}

	if(newClientSock.receive(joinReqPkt) != sf::Socket::Status::Done)
	{
		newClientSock.disconnect();
		return;
	}

	uint8_t type;
	joinReqPkt >> type;
	if(type != uint8_t(ReliablePktType::JOIN_REQ))
	{
		newClientSock.disconnect();
		return;
	}

	uint32_t protoVer;
	joinReqPkt >> protoVer;
	if(protoVer != PROTOCOL_VERSION)
	{
		newClientSock.disconnect();
		return;
	}

	LobbyPlayer p;
	joinReqPkt >> p.name >> p.gamePort;

	// Assign ID
	uint32_t assignedId = 0;
	int slotIndex = -1;

	// Reuse slot if available
	for(size_t i = 0; i < m_slots.size(); i++)
	{
		if(!m_slots[i].bValid)
		{
			assignedId = m_slots[i].id;
			slotIndex = static_cast<int>(i);
			break;
		}
	}
	if(assignedId == 0)
		assignedId = m_nextId++;

	p.id = assignedId;
	p.udpAddr = newClientSock.getRemoteAddress().value();
	p.bValid = true;

	deduplicatePlayerName(p.name);

	sf::Packet joinAckPkt = createPkt(ReliablePktType::JOIN_ACK);
	joinAckPkt << p.id << p.name;

	if(checkedSend(newClientSock, joinAckPkt) != sf::Socket::Status::Done)
	{
		newClientSock.disconnect();
		return;
	}

	// 2. Add to Poll Group
	p.tcpSocket = std::move(newClientSock);
	p.tcpSocket.setBlocking(false);
	m_multiSock.add(p.tcpSocket);

	if(slotIndex >= 0)
	{
		m_slots[slotIndex] = std::move(p);
	}
	else
	{
		m_slots.push_back(std::move(p));
	}

	SPDLOG_LOGGER_INFO(spdlog::get("Server"), "Player {} (id {}) joined lobby", p.name, p.id);
	broadcastLobbyUpdate();
}

void LobbyServer::handleClient(LobbyPlayer &p)
{
	sf::Packet rxPkt;
	sf::Socket::Status status = p.tcpSocket.receive(rxPkt);

	if(status == sf::Socket::Status::Disconnected)
	{
		SPDLOG_LOGGER_INFO(spdlog::get("Server"), "Player {} (id {}) disconnected", p.name, p.id);
		m_multiSock.remove(p.tcpSocket);
		p.tcpSocket.disconnect();
		p.bValid = false;
		p.bReady = false;

		// If host (player 1) disconnects, stop the server
		if(p.id == 1)
		{
			SPDLOG_LOGGER_INFO(spdlog::get("Server"), "Host disconnected, stopping server...");
			stop();
			return;
		}

		broadcastLobbyUpdate();
		return;
	}

	if(status == sf::Socket::Status::Done)
	{
		uint8_t type;
		rxPkt >> type;
		if(type == static_cast<uint8_t>(ReliablePktType::LOBBY_READY))
		{
			uint32_t clientId;
			rxPkt >> clientId;
			if(!p.bReady)
			{
				p.bReady = true;
				SPDLOG_LOGGER_INFO(spdlog::get("Server"), "Player {} (id {}) is READY", p.name, p.id);
				broadcastLobbyUpdate();
			}
		}
	}
}

void LobbyServer::startGame(WorldState &worldState)
{
    static std::mt19937_64 rng{std::random_device{}()};

    auto spawns = worldState.getMap().getSpawns();

	if(spawns.empty())
	{
		SPDLOG_LOGGER_WARN(spdlog::get("Server"), "No spawn points found in Map! Using default (100,100).");
		spawns.push_back({100.f, 100.f});
	}

    std::shuffle(spawns.begin(), spawns.end(), rng);

    sf::Packet startPkt = createPkt(ReliablePktType::GAME_START);

    std::vector<PlayerState> activePlayers;
    activePlayers.reserve(MAX_PLAYERS);

    size_t spawnIndex = 0;

    for(auto const &lobbyPlayer : m_slots)
    {
       if(!lobbyPlayer.bValid)
          continue;

       sf::Vector2f const spawnPos = spawns[spawnIndex % spawns.size()];

       PlayerState ps(lobbyPlayer.id, spawnPos);

       // inject lobby data
       ps.m_name = lobbyPlayer.name;
       ps.m_kills = lobbyPlayer.totalKills;
       ps.m_deaths = lobbyPlayer.totalDeaths;

       worldState.setPlayer(ps);

       activePlayers.push_back(ps);
       spawnIndex++;
    }

    startPkt << activePlayers.size();

    for(auto const &playerState : activePlayers)
    {
       startPkt << playerState.getPosition() << playerState.getRotation();
    }

    for(auto &p : m_slots)
    {
       if(!p.bValid)
          continue;

       if(checkedSend(p.tcpSocket, startPkt) != sf::Socket::Status::Done)
       {
          SPDLOG_LOGGER_ERROR(spdlog::get("Server"), "Failed to send GAME_START to {} (id {})", p.name, p.id);
       }
    }
}

void LobbyServer::endGame(EntityId winner)
{
	m_cReady = 0;
	for(auto &c : m_slots)
		c.bReady = false;

	sf::Packet gameEndPkt = createPkt(ReliablePktType::GAME_END);
	gameEndPkt << winner;

	uint32_t playerCount = 0;
	for(auto const &p : m_slots)
	{
		if(p.bValid)
			playerCount++;
	}
	gameEndPkt << playerCount;

	for(auto const &p : m_slots)
	{
		if(p.bValid)
		{
			gameEndPkt << p.id << p.name << static_cast<int32_t>(p.totalKills) << static_cast<int32_t>(p.totalDeaths);
		}
	}

	for(auto &p : m_slots)
	{
		if(p.bValid)
			checkedSend(p.tcpSocket, gameEndPkt);
	}
}

void LobbyServer::broadcastLobbyUpdate()
{
	uint32_t numPlayers = 0;
	for(auto const &p : m_slots)
		if(p.bValid)
			numPlayers++;

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
}

void LobbyServer::updatePlayerStats(std::array<PlayerState, MAX_PLAYERS> const &playerStates)
{
	for(auto &slot : m_slots)
	{
		if(!slot.bValid)
			continue;

		for(auto const &ps : playerStates)
		{
			if(ps.m_id == slot.id)
			{
				slot.totalKills = ps.getKills();
				slot.totalDeaths = ps.getDeaths();
				SPDLOG_LOGGER_INFO(spdlog::get("Server"), "Updated stats for {} (id {}): {} kills, {} deaths",
				                   slot.name, slot.id, slot.totalKills, slot.totalDeaths);
				break;
			}
		}
	}
}

void LobbyServer::resetLobbyState()
{
	for(auto &p : m_slots)
	{
		if(p.bValid)
		{
			p.bReady = false;
		}
	}
	m_cReady = 0;
	m_gameInProgress = false;
	SPDLOG_LOGGER_INFO(spdlog::get("Server"), "Lobby state reset: all players unmarked as ready");

	sf::sleep(sf::milliseconds(100));
	broadcastLobbyUpdate();
}