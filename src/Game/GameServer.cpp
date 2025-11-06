#include "GameServer.h"
#include "Lobby/LobbyServer.h"
#include <spdlog/spdlog.h>

GameServer::GameServer(LobbyServer &lobbyServer, uint16_t gamePort,
                       std::shared_ptr<spdlog::logger> const &logger)
	: m_gamePort(gamePort),
	  m_world(WINDOW_DIMf),
	  m_numAlive(MAX_PLAYERS),
	  m_logger(logger),
	  m_lobby(lobbyServer)
{
	for(auto const &p : lobbyServer.m_slots)
	{
		assert(p.gamePort != gamePort); // No client can use our port, else get collisions on local multiplayer
	}
	sf::Socket::Status status = m_gameSock.bind(m_gamePort);
	if(status != sf::Socket::Status::Done)
	{
		SPDLOG_LOGGER_ERROR(logger, "Failed to bind UDP port {}: {}", m_gamePort, int(status));
		throw std::runtime_error("Failed to bind UDP port");
	}
	m_gameSock.setBlocking(false);
}

GameServer::~GameServer()
{
	m_gameSock.unbind();
}

PlayerState *GameServer::matchLoop()
{
	while(m_numAlive > 1)
	{
		processPackets();

		// maintain fixed tick rate updates
		float dt = m_tickClock.restart().asSeconds();
		if(dt < UNRELIABLE_TICK_RATE)
			sf::sleep(sf::seconds(UNRELIABLE_TICK_RATE - dt));
		m_world.update(UNRELIABLE_TICK_RATE);

		floodWorldState();

		int numAlive = 0;
		PlayerState *pWinner = nullptr;
		for(auto &p : m_world.getPlayers())
		{
			if(p.isAlive())
			{
				numAlive++;
				pWinner = &p;
			}
		}
		if(numAlive == 1)
		{
			SPDLOG_LOGGER_INFO(m_logger, "Game ended, winner {}.", pWinner->m_id);
			return pWinner;
		}
		if(numAlive == 0)
		{
			// Technically possible that all players die in the same tick
			SPDLOG_LOGGER_WARN(m_logger, "Game ended in a draw.");
			break;
		}
	}
	return nullptr;
}

void GameServer::processPackets()
{
	sf::Packet rxPkt;
	std::optional<sf::IpAddress> srcAddrOpt;
	uint16_t srcPort;
	if(m_gameSock.receive(rxPkt, srcAddrOpt, srcPort) == sf::Socket::Status::Done)
	{
		uint8_t type;
		rxPkt >> type;
		switch(UnreliablePktType(type))
		{
		case UnreliablePktType::MOVE: {
			uint32_t clientId;
			sf::Vector2f posDelta;
			sf::Angle rot;
			rxPkt >> clientId;
			rxPkt >> posDelta;
			rxPkt >> rot; // TODO the server does not care about it - ok to remove ?
			auto const &states = m_world.getPlayers();
			if(clientId <= states.size() && m_lobby.m_slots[clientId - 1].bValid)
			{
				PlayerState &ps = m_world.getPlayerById(clientId);

				ps.moveOn(m_world.getMap(), posDelta);
				SPDLOG_LOGGER_INFO(m_logger, "Recv MOVE id {} pos=({},{}), rotDeg={}", clientId, ps.m_pos.x,
				                   ps.m_pos.y, ps.m_rot.asDegrees());
			}
			else
				SPDLOG_LOGGER_WARN(m_logger, "Dropping packet from invalid player id {}", clientId);
			break;
		}
		default:
			SPDLOG_LOGGER_WARN(m_logger, "Unhandled unreliable packet type: {}, src={}:{}",
			                   type, srcAddrOpt.value().toString(), srcPort);
		}
	}
}

void GameServer::floodWorldState()
{
	int numAlive = 0;
	for(auto &p : m_world.getPlayers())
		numAlive += p.isAlive();
	if(numAlive <= 1)
	{
		SPDLOG_LOGGER_INFO(m_logger, "Game ended, alive players: {}. Returning to lobby.", numAlive);
		return;
	}

	// flood snapshots to all known clients
	sf::Packet snapPkt = createPkt(UnreliablePktType::SNAPSHOT);
	m_world.serialize(snapPkt);
	for(LobbyPlayer &p : m_lobby.m_slots)
	{
		if(!p.bValid)
			continue;
		if(m_gameSock.send(snapPkt, p.udpAddr, p.gamePort) != sf::Socket::Status::Done)
			SPDLOG_LOGGER_ERROR(m_logger, "Failed to send snapshot to {}:{}",
		                    p.udpAddr.toString(), p.gamePort);
	}
}
