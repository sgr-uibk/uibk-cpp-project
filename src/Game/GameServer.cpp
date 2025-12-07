#include "GameServer.h"
#include "Lobby/LobbyServer.h"
#include "GameConfig.h"
#include <spdlog/spdlog.h>

GameServer::GameServer(LobbyServer &lobbyServer, uint16_t gamePort)
	: m_gamePort(gamePort), m_world(WorldState::fromTiledMap("map/arena.json")), m_lobby(lobbyServer)
{
	for(auto const &p : lobbyServer.m_slots)
	{
		assert(p.gamePort != gamePort); // No client can use our port, else get collisions on local multiplayer
	}
	sf::Socket::Status status = m_gameSock.bind(m_gamePort);
	if(status != sf::Socket::Status::Done)
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("Server"), "Failed to bind UDP port {}: {}", m_gamePort, int(status));
		throw std::runtime_error("Failed to bind UDP port");
	}
	m_gameSock.setBlocking(false);
	m_itemSpawnClock.start();

	m_nextItemSpawnIndex = 0;
}

GameServer::~GameServer()
{
	m_gameSock.unbind();
}

PlayerState *GameServer::matchLoop()
{
	PlayerState *pWinner = nullptr;
	while(true)
	{
		// Clear wall deltas from previous tick
		m_world.clearWallDeltas();

		processPackets();
		checkPlayerConnections();

		// maintain fixed tick rate updates
		float dt = m_tickClock.getElapsedTime().asSeconds();
		sf::sleep(sf::seconds(UNRELIABLE_TICK_TIME - dt));
		m_tickClock.restart();
		++m_authTick;
		m_world.update(UNRELIABLE_TICK_TIME);

		spawnItems();

		m_world.checkProjectilePlayerCollisions();
		m_world.checkProjectileWallCollisions();
		m_world.checkPlayerItemCollisions();
		m_world.checkPlayerPlayerCollisions();
		m_world.removeInactiveProjectiles();
		m_world.removeInactiveItems();

		size_t cAlive = 0;
		for(auto &p : m_world.getPlayers())
		{
			bool const bAlive = p.isAlive();
			cAlive += bAlive;
			if(bAlive)
				pWinner = &p;
		}

		switch(cAlive)
		{
		case 1:
			SPDLOG_LOGGER_INFO(spdlog::get("Server"), "Game ended, winner {}.", pWinner->m_id);
			return pWinner;
		case 0: // Technically possible that all players die in the same tick
			SPDLOG_LOGGER_WARN(spdlog::get("Server"), "Game ended in a draw.");
			return nullptr;
		default:
			floodWorldState();
		}
	}
}

void GameServer::processPackets()
{
	sf::Packet rxPkt;
	std::optional<sf::IpAddress> srcAddrOpt;
	uint16_t srcPort;
	while(checkedReceive(m_gameSock, rxPkt, srcAddrOpt, srcPort) == sf::Socket::Status::Done)
	{
		uint8_t type;
		Tick tick;
		rxPkt >> type >> tick;
		switch(UnreliablePktType(type))
		{
		case UnreliablePktType::MOVE: {
			uint32_t clientId;
			sf::Vector2f posDelta;
			sf::Angle cannonAngle;
			rxPkt >> clientId >> posDelta >> cannonAngle;
			auto const &states = m_world.getPlayers();
			if(clientId <= states.size() && m_lobby.m_slots[clientId - 1].bValid)
			{
				if(m_lastClientTicks[clientId - 1] >= tick)
				{
					SPDLOG_LOGGER_WARN(spdlog::get("Server"), "MOVE: Outdated pkt from {}, (know {}, got {})", clientId,
					                   m_lastClientTicks[clientId - 1], tick);
					break;
				}
				m_lastClientTicks[clientId - 1] = tick;
				PlayerState &ps = m_world.getPlayerById(clientId);
				ps.moveOn(m_world.getMap(), posDelta);
				ps.setCannonRotation(cannonAngle);
			}
			else
				SPDLOG_LOGGER_WARN(spdlog::get("Server"), "MOVE: Dropping packet from invalid player id {}", clientId);
			break;
		}
		case UnreliablePktType::SHOOT: {
			uint32_t clientId;
			sf::Angle cannonAngle;
			rxPkt >> clientId >> cannonAngle;
			auto const &states = m_world.getPlayers();
			if(clientId <= states.size() && m_lobby.m_slots[clientId - 1].bValid)
			{
				PlayerState &ps = m_world.getPlayerById(clientId);

				if(ps.canShoot())
				{
					ps.shoot();
					ps.setCannonRotation(cannonAngle);

					// spawn projectile at cannon tip
					sf::Vector2f tankCenter = ps.getPosition() + sf::Vector2f(PlayerState::logicalDimensions / 2.f);
					sf::Vector2f cannonOffset(std::sin(cannonAngle.asRadians()) * GameConfig::Player::CANNON_LENGTH,
					                          -std::cos(cannonAngle.asRadians()) * GameConfig::Player::CANNON_LENGTH);
					sf::Vector2f position = tankCenter + cannonOffset;

					// calculate velocity from cannon angle
					sf::Vector2f velocity(std::sin(cannonAngle.asRadians()) * GameConfig::Projectile::SPEED,
					                      -std::cos(cannonAngle.asRadians()) * GameConfig::Projectile::SPEED);

					// Apply damage multiplier from powerups
					int damage = GameConfig::Projectile::BASE_DAMAGE * ps.getDamageMultiplier();
					m_world.addProjectile(position, velocity, clientId, damage);
					SPDLOG_LOGGER_INFO(spdlog::get("Server"), "Player {} shoots at t={}, angle={} (damage={})",
					                   clientId, tick, cannonAngle.asDegrees(), damage);
				}
			}
			else
				SPDLOG_LOGGER_WARN(spdlog::get("Server"), "Dropping SHOOT packet from invalid player id {}", clientId);
			break;
		}
		case UnreliablePktType::USE_ITEM: {
			uint32_t clientId;
			size_t slot;
			rxPkt >> clientId >> slot;
			auto const &states = m_world.getPlayers();
			if(clientId <= states.size() && m_lobby.m_slots[clientId - 1].bValid)
			{
				PlayerState &ps = m_world.getPlayerById(clientId);
				ps.useItem(slot);
				SPDLOG_LOGGER_INFO(spdlog::get("Server"), "Player {} used item at t={}, slot #{}", clientId, tick,
				                   slot);
			}
			else
				SPDLOG_LOGGER_WARN(spdlog::get("Server"), "Dropping USE_ITEM packet from invalid player id {}",
				                   clientId);
			break;
		}
		default:
			std::string srcAddr = srcAddrOpt.has_value() ? srcAddrOpt.value().toString() : std::string("?");
			SPDLOG_LOGGER_WARN(spdlog::get("Server"), "Unhandled unreliable packet type: {}, src={}:{}", type, srcAddr,
			                   srcPort);
		}
	}
}

void GameServer::spawnItems()
{
	if(m_itemSpawnClock.getElapsedTime().asSeconds() > GameConfig::ItemSpawn::SPAWN_INTERVAL &&
	   m_world.getItems().size() < GameConfig::ItemSpawn::MAX_ITEMS_ON_MAP)
	{
		m_itemSpawnClock.restart();

		auto const &itemSpawnZones = m_world.getMap().getItemSpawnZones();

		if(itemSpawnZones.empty())
		{
			float x = 100.f + static_cast<float>(rand() % 600);
			float y = 100.f + static_cast<float>(rand() % 400);
			sf::Vector2f position(x, y);
			PowerupType type = static_cast<PowerupType>(1 + (rand() % 5));
			m_world.addItem(position, type);
			SPDLOG_LOGGER_INFO(spdlog::get("Server"), "Spawned random powerup type {} at ({}, {})",
			                   static_cast<int>(type), x, y);
		}
		else
		{
			ItemSpawnZone const &zone = itemSpawnZones[m_nextItemSpawnIndex];
			m_world.addItem(zone.position, zone.itemType);
			SPDLOG_LOGGER_INFO(spdlog::get("Server"), "Spawned powerup type {} at ({}, {}) from map spawn zone",
			                   static_cast<int>(zone.itemType), zone.position.x, zone.position.y);

			m_nextItemSpawnIndex = (m_nextItemSpawnIndex + 1) % itemSpawnZones.size();
		}
	}
}

void GameServer::floodWorldState()
{
	// flood snapshots to all known clients
	sf::Packet snapPkt = createTickedPkt(UnreliablePktType::SNAPSHOT, m_authTick);
	snapPkt << m_lastClientTicks;
	m_world.serialize(snapPkt);
	for(LobbyPlayer &p : m_lobby.m_slots)
	{
		if(!p.bValid)
			continue;
		if(checkedSend(m_gameSock, snapPkt, p.udpAddr, p.gamePort) != sf::Socket::Status::Done)
		{
			SPDLOG_LOGGER_ERROR(spdlog::get("Server"), "Failed to send snapshot to {}:{}", p.udpAddr.toString(),
			                    p.gamePort);
		}
	}
}

void GameServer::checkPlayerConnections()
{
	// check TCP connection status for all players
	// mark disconnected players as dead
	for(LobbyPlayer &lobbySlot : m_lobby.m_slots)
	{
		if(!lobbySlot.bValid)
			continue;

		sf::Packet testPkt;
		lobbySlot.tcpSocket.setBlocking(false);
		sf::Socket::Status status = lobbySlot.tcpSocket.receive(testPkt);

		if(status == sf::Socket::Status::Disconnected)
		{
			SPDLOG_LOGGER_WARN(spdlog::get("Server"), "Player {} (id {}) disconnected during game, marking as dead",
			                   lobbySlot.name, lobbySlot.id);

			PlayerState &playerState = m_world.getPlayerById(lobbySlot.id);
			if(playerState.isAlive())
			{
				playerState.takeDamage(playerState.getHealth());
				SPDLOG_LOGGER_INFO(spdlog::get("Server"), "Player {} eliminated due to disconnect", lobbySlot.id);
			}
		}
	}
}
