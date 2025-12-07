#include "GameServer.h"
#include "Lobby/LobbyServer.h"
#include "GameConfig.h"
#include <spdlog/spdlog.h>

GameServer::GameServer(LobbyServer &lobbyServer, uint16_t gamePort, std::shared_ptr<spdlog::logger> const &logger)
	: m_gamePort(gamePort), m_world(WorldState::fromTiledMap("map/arena.json")), m_numAlive(MAX_PLAYERS),
	  m_logger(logger), m_lobby(lobbyServer)
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

	m_nextItemSpawnIndex = 0;
}

GameServer::~GameServer()
{
	m_gameSock.unbind();
}

PlayerState *GameServer::matchLoop()
{
	while(m_numAlive > 1 && !m_lobby.isShutdownRequested())
	{
		// Clear wall deltas from previous tick
		m_world.clearWallDeltas();

		processPackets();
		checkPlayerConnections();

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

	if(m_lobby.isShutdownRequested())
	{
		SPDLOG_LOGGER_INFO(m_logger, "Game ended due to server shutdown request");
	}

	return nullptr;
}

void GameServer::processPackets()
{
	// process ALL available packets
	while(true)
	{
		sf::Packet rxPkt;
		std::optional<sf::IpAddress> srcAddrOpt;
		uint16_t srcPort;

		if(checkedReceive(m_gameSock, rxPkt, srcAddrOpt, srcPort) != sf::Socket::Status::Done)
			break;

		uint8_t type;
		rxPkt >> type;
		switch(UnreliablePktType(type))
		{
		case UnreliablePktType::MOVE: {
			uint32_t clientId;
			sf::Vector2f posDelta;
			sf::Angle cannonAngle;
			rxPkt >> clientId;
			rxPkt >> posDelta;
			rxPkt >> cannonAngle;
			auto const &states = m_world.getPlayers();
			if(clientId <= states.size() && m_lobby.m_slots[clientId - 1].bValid)
			{
				PlayerState &ps = m_world.getPlayerById(clientId);

				ps.moveOn(m_world.getMap(), posDelta);
				ps.setCannonRotation(cannonAngle);
				SPDLOG_LOGGER_INFO(m_logger, "Recv MOVE id {} pos=({},{}), rotDeg={}, cannonDeg={}", clientId,
				                   ps.m_pos.x, ps.m_pos.y, ps.m_rot.asDegrees(), ps.m_cannonRot.asDegrees());
			}
			else
				SPDLOG_LOGGER_WARN(m_logger, "Dropping MOVE packet from invalid player id {}", clientId);
			break;
		}
		case UnreliablePktType::SHOOT: {
			uint32_t clientId;
			sf::Angle cannonAngle;
			rxPkt >> clientId;
			rxPkt >> cannonAngle;
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
					SPDLOG_LOGGER_INFO(m_logger, "Player {} shoots at angle {} (damage={})", clientId,
					                   cannonAngle.asDegrees(), damage);
				}
			}
			else
				SPDLOG_LOGGER_WARN(m_logger, "Dropping SHOOT packet from invalid player id {}", clientId);
			break;
		}
		case UnreliablePktType::SELECT_SLOT: {
			uint32_t clientId;
			int32_t slot;
			rxPkt >> clientId;
			rxPkt >> slot;
			auto const &states = m_world.getPlayers();
			if(clientId <= states.size() && m_lobby.m_slots[clientId - 1].bValid)
			{
				PlayerState &ps = m_world.getPlayerById(clientId);
				ps.setSelectedSlot(slot);
				SPDLOG_LOGGER_DEBUG(m_logger, "Player {} selected slot {}", clientId, slot);
			}
			else
				SPDLOG_LOGGER_WARN(m_logger, "Dropping SELECT_SLOT packet from invalid player id {}", clientId);
			break;
		}
		case UnreliablePktType::USE_ITEM: {
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

		const auto &itemSpawnZones = m_world.getMap().getItemSpawnZones();

		if(itemSpawnZones.empty())
		{
			float x = 100.f + static_cast<float>(rand() % 600);
			float y = 100.f + static_cast<float>(rand() % 400);
			sf::Vector2f position(x, y);
			PowerupType type = static_cast<PowerupType>(1 + (rand() % 5));
			m_world.addItem(position, type);
			SPDLOG_LOGGER_INFO(m_logger, "Spawned random powerup type {} at ({}, {})", static_cast<int>(type), x, y);
		}
		else
		{
			const ItemSpawnZone &zone = itemSpawnZones[m_nextItemSpawnIndex];
			m_world.addItem(zone.position, zone.itemType);
			SPDLOG_LOGGER_INFO(m_logger, "Spawned powerup type {} at ({}, {}) from map spawn zone",
			                   static_cast<int>(zone.itemType), zone.position.x, zone.position.y);

			m_nextItemSpawnIndex = (m_nextItemSpawnIndex + 1) % itemSpawnZones.size();
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
		if(checkedSend(m_gameSock, snapPkt, p.udpAddr, p.gamePort) != sf::Socket::Status::Done)
		{
			SPDLOG_LOGGER_ERROR(m_logger, "Failed to send snapshot to {}:{}", p.udpAddr.toString(), p.gamePort);
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
			SPDLOG_LOGGER_WARN(m_logger, "Player {} (id {}) disconnected during game, marking as dead", lobbySlot.name,
			                   lobbySlot.id);

			PlayerState &playerState = m_world.getPlayerById(lobbySlot.id);
			if(playerState.isAlive())
			{
				playerState.takeDamage(playerState.getHealth());
				SPDLOG_LOGGER_INFO(m_logger, "Player {} eliminated due to disconnect", lobbySlot.id);
			}
		}
	}
}
