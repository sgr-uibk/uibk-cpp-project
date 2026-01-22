#include "LobbyServer.h"
#include <cassert>
#include <memory>
#include <random>
#include <spdlog/spdlog.h>
#include "GameConfig.h"
#include "Map/MapParser.h"

LobbyServer::LobbyServer(uint16_t tcpPort)
{
	if(m_listener.listen(tcpPort) != sf::Socket::Status::Done)
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("Server"), "Failed to listen on TCP {}", tcpPort);
		throw std::runtime_error("Port " + std::to_string(tcpPort) + " is already in use.");
	}
	m_listener.setBlocking(false);
	m_multiSock.add(m_listener);
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

void LobbyServer::tickStep()
{
    if (m_multiSock.wait(sf::milliseconds(1))) 
    {
        if (m_multiSock.isReady(m_listener))
            acceptNewClient();

        for (auto& p : m_slots)
            if (p.bValid)
                handleClient(p);
    }
}

bool LobbyServer::readyToStart() const
{
    return m_cReady == MAX_PLAYERS;
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
	auto const remoteAddrOpt = newClientSock.getRemoteAddress();
	assert(remoteAddrOpt.has_value()); // Know exists from status done.

	EntityId tentativeId = 0;
	while(tentativeId < MAX_PLAYERS && m_slots[tentativeId].bValid)
		++tentativeId;

	if(tentativeId++ == MAX_PLAYERS)
	{
		SPDLOG_LOGGER_WARN(spdlog::get("Server"), "Maximum lobby size reached, rejecting player.");
		newClientSock.disconnect();
		return;
	}

	LobbyPlayer p{
		.id = tentativeId, .endpoint = {.ip = *remoteAddrOpt, .port = 0}, .tcpSocket = std::move(newClientSock)};

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
	joinReqPkt >> p.name >> p.endpoint.port;
	SPDLOG_LOGGER_INFO(spdlog::get("Server"), "Client id {}, udpPort {}, requested name '{}' ", p.id, p.endpoint.port,
	                   p.name);
	assert(p.id && p.endpoint.port);

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
	m_slots[p.id - 1] = std::move(p);
	broadcastLobbyUpdate();
}

uint32_t LobbyServer::findClient(Endpoint remote) const
{
	uint32_t clientId = 0;
	while(clientId < m_slots.size())
	{
		if(m_slots[clientId++].endpoint == remote)
			return clientId;
	}
	SPDLOG_LOGGER_WARN(spdlog::get("Server"), "Unknown client: {}:{}", remote.ip.toString(), remote.port);
	return 0;
}

void LobbyServer::handleClient(LobbyPlayer &p)
{
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
			if(!p.bReady)
			{
				p.bReady = true;
				m_cReady++;
			}
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

template <std::size_t Sz, std::size_t... I>
std::array<PlayerState, Sz> make_player_init_impl(std::vector<sf::Vector2f> const &spawns, std::mt19937_64 &rng,
                                                  std::index_sequence<I...>)
{
	// compute rotation per index by calling rng; evaluated left-to-right due to comma fold below
	// we must ensure rng is called exactly once per element; use a lambda to call rng and produce rot
	auto make_one = [&](auto idx) {
		(void)idx;
		sf::Angle rot = sf::radians(float(rng()) / float(rng.max()));
		return PlayerState(static_cast<int>(idx + 1), spawns[idx], rot);
	};

	// Expand to initialize the array. Use a lambda to force left-to-right evaluation order.
	return {make_one(I)...};
}

template <std::size_t Sz>
std::array<PlayerState, Sz> make_player_init(std::vector<sf::Vector2f> const &spawns, std::mt19937_64 &rng)
{
	return make_player_init_impl<Sz>(spawns, rng, std::make_index_sequence<Sz>{});
}

WorldState LobbyServer::startGame()
{
	// Select map (for now always use default map)
	int mapIndex = Maps::DEFAULT_MAP_INDEX;
	std::string mapPath = Maps::getMapPath(mapIndex);

	// Read only spawn points (no full map loading)
	auto spawns = MapParser::parseSpawnsOnly(mapPath);

	if(spawns.size() < MAX_PLAYERS)
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("Server"), "Map {} has insufficient spawn points ({} < {})", mapPath,
		                    spawns.size(), MAX_PLAYERS);
		// Fallback: use default positions
		spawns.clear();
		for(size_t i = 0; i < MAX_PLAYERS; ++i)
			spawns.push_back(sf::Vector2f(100.f + i * 100.f, 100.f + i * 100.f));
	}

	static std::mt19937_64 rng{}; // deterministic spawn points
	std::ranges::shuffle(spawns, rng);

	auto playerInit = make_player_init<MAX_PLAYERS>(spawns, rng);

	sf::Packet startPkt = createPkt(ReliablePktType::GAME_START);
	startPkt << mapIndex; // Send map index to clients
	serializeFromArray(startPkt, playerInit, std::make_index_sequence<MAX_PLAYERS>{});

	for(auto &lp : m_slots)
	{ // Distribute spawn points with GAME_START pkt
		if(!lp.bValid)
			continue;
		if(checkedSend(lp.tcpSocket, startPkt) != sf::Socket::Status::Done)
			SPDLOG_LOGGER_ERROR(spdlog::get("Server"), "Failed to send GAME_START to {} (id {})", lp.name, lp.id);
	}
	return WorldState{mapIndex, std::move(playerInit)};
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
}
