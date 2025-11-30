#include "GameServer.h"
#include "Lobby/LobbyServer.h"
#include "GameConfig.h"
#include <spdlog/spdlog.h>

GameServer::GameServer(LobbyServer &lobbyServer, uint16_t gamePort, std::shared_ptr<spdlog::logger> const &logger)
	: m_gamePort(gamePort), m_world(WINDOW_DIMf), m_numAlive(MAX_PLAYERS), m_logger(logger), m_lobby(lobbyServer)
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
	m_itemSpawnClock.start();
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

		spawnItems();

		m_world.checkProjectilePlayerCollisions();
		m_world.checkProjectileWallCollisions();
		m_world.checkPlayerItemCollisions();
		m_world.checkPlayerPlayerCollisions();
		m_world.removeInactiveProjectiles();
		m_world.removeInactiveItems();

		floodWorldState();

		int numAlive = 0;
		PlayerState *pWinner = nullptr;
		for(auto &p : m_world.getPlayers())
		{
			if(p.isAlive())
			{
				numAlive++;
				pWinner = &p;
				SPDLOG_LOGGER_DEBUG(m_logger, "Player {} is alive: hp={}, pos=({},{})", p.m_id, p.getHealth(),
				                    p.getPosition().x, p.getPosition().y);
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
	while(checkedReceive(m_gameSock, rxPkt, srcAddrOpt, srcPort) == sf::Socket::Status::Done)
	{
		uint8_t type;
		rxPkt >> type;
		switch(UnreliablePktType(type))
		{
		case UnreliablePktType::MOVE: {
			uint32_t clientId;
			sf::Vector2f posDelta;
			rxPkt >> clientId;
			rxPkt >> posDelta;
			auto const &states = m_world.getPlayers();
			if(clientId <= states.size() && m_lobby.m_slots[clientId - 1].bValid)
			{
				PlayerState &ps = m_world.getPlayerById(clientId);

				ps.moveOn(m_world.getMap(), posDelta);
				SPDLOG_LOGGER_INFO(m_logger, "Recv MOVE id {} pos=({},{}), rotDeg={}", clientId, ps.m_pos.x, ps.m_pos.y,
				                   ps.m_rot.asDegrees());
			}
			else
				SPDLOG_LOGGER_WARN(m_logger, "Dropping MOVE packet from invalid player id {}", clientId);
			break;
		}
		case UnreliablePktType::SHOOT: {
			uint32_t clientId;
			sf::Angle aimAngle;
			rxPkt >> clientId;
			rxPkt >> aimAngle;
			auto const &states = m_world.getPlayers();
			if(clientId <= states.size() && m_lobby.m_slots[clientId - 1].bValid)
			{
				PlayerState &ps = m_world.getPlayerById(clientId);

				if(ps.canShoot())
				{
					ps.shoot();

					// spawn projectile at player position
					sf::Vector2f position = ps.getPosition();

					// calculate velocity from aim angle
					sf::Vector2f velocity(std::sin(aimAngle.asRadians()) * GameConfig::Projectile::SPEED,
					                      -std::cos(aimAngle.asRadians()) * GameConfig::Projectile::SPEED);

					// Apply damage multiplier from powerups
					int damage = GameConfig::Projectile::BASE_DAMAGE * ps.getDamageMultiplier();
					m_world.addProjectile(position, velocity, clientId, damage);
					SPDLOG_LOGGER_INFO(m_logger, "Player {} shoots at angle {} (damage={})", clientId,
					                   aimAngle.asDegrees(), damage);
				}
			}
			else
				SPDLOG_LOGGER_WARN(m_logger, "Dropping SHOOT packet from invalid player id {}", clientId);
			break;
		}
		case UnreliablePktType::USE_ITEM: { // todo select based on server state
			uint32_t clientId;
			rxPkt >> clientId;
			auto const &states = m_world.getPlayers();
			if(clientId <= states.size() && m_lobby.m_slots[clientId - 1].bValid)
			{
				PlayerState &ps = m_world.getPlayerById(clientId);
				ps.useSelectedItem();
				SPDLOG_LOGGER_INFO(m_logger, "Player {} used item from slot {}", clientId, ps.getSelectedSlot());
			}
			else
				SPDLOG_LOGGER_WARN(m_logger, "Dropping USE_ITEM packet from invalid player id {}", clientId);
			break;
		}
		default:
			std::string srcAddr = srcAddrOpt.has_value() ? srcAddrOpt.value().toString() : std::string("?");
			SPDLOG_LOGGER_WARN(m_logger, "Unhandled unreliable packet type: {}, src={}:{}", type, srcAddr, srcPort);
		}
	}
}

void GameServer::spawnItems()
{
	if(m_itemSpawnClock.getElapsedTime().asSeconds() > GameConfig::ItemSpawn::SPAWN_INTERVAL &&
	   m_world.getItems().size() < GameConfig::ItemSpawn::MAX_ITEMS_ON_MAP)
	{
		m_itemSpawnClock.restart();

		float x = 100.f + static_cast<float>(rand() % 600);
		float y = 100.f + static_cast<float>(rand() % 400);
		sf::Vector2f position(x, y);

		PowerupType type = static_cast<PowerupType>(1 + (rand() % 5));

		m_world.addItem(position, type);
		SPDLOG_LOGGER_INFO(m_logger, "Spawned powerup type {} at ({}, {})", static_cast<int>(type), x, y);
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
		if(checkedSend(m_gameSock, snapPkt, p.udpAddr, p.gamePort) != sf::Socket::Status::Done)
		{
			SPDLOG_LOGGER_ERROR(m_logger, "Failed to send snapshot to {}:{}", p.udpAddr.toString(), p.gamePort);
		}
	}
}
