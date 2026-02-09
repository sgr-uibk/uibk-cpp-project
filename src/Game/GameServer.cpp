#include "GameServer.h"
#include "GameConfig.h"
#include "Lobby/LobbyServer.h"
#include "World/WorldClient.h"
#include <algorithm>
#include <spdlog/spdlog.h>

using namespace GameConfig::ItemSpawn;

GameServer::GameServer(LobbyServer &lobbyServer, uint16_t const gamePort, WorldState const &wsInit)
	: m_gamePort(gamePort), m_world(wsInit), m_lobby(lobbyServer)
{
	// ensure no client uses our UDP port to avoid collisions in local multiplayer
	assert(std::ranges::none_of(lobbyServer.m_slots,
	                            [gamePort](auto const &slot) { return slot.endpoint.port == gamePort; }));
	sf::Socket::Status status = m_gameSock.bind(m_gamePort);
	if(status != sf::Socket::Status::Done)
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("Server"), "Failed to bind UDP port {}: {}", m_gamePort, int(status));
		throw std::runtime_error("Failed to bind UDP port");
	}
	m_gameSock.setBlocking(false);
	m_itemSpawnClock.start();
	m_safeZoneClock.start();

	m_nextItemSpawnIndex = 0;
}

GameServer::~GameServer()
{
	m_gameSock.unbind();
}

PlayerState *GameServer::matchLoop(std::atomic<bool> const &bForceEnd)
{
	PlayerState *pWinner = nullptr;
	while(!bForceEnd)
	{
		m_world.clearWallDeltas();

		processPackets();

		float const dt = m_tickClock.getElapsedTime().asSeconds();
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

		if(!m_world.m_safeZone.isActive &&
		   (m_safeZoneClock.getElapsedTime().asSeconds() >= GameConfig::SafeZone::INITIAL_DELAY || cAlive <= 2))
		{
			m_world.m_safeZone.isActive = true;
			SPDLOG_LOGGER_INFO(spdlog::get("Server"), "Safe zone activated: radius {} -> {}",
			                   m_world.m_safeZone.currentRadius, m_world.m_safeZone.targetRadius);
		}

		switch(cAlive)
		{
		case 1:
			SPDLOG_LOGGER_INFO(spdlog::get("Server"), "Game ended, winner {}.", pWinner->m_id);
			return pWinner;
		case 0:
			SPDLOG_LOGGER_WARN(spdlog::get("Server"), "Game ended in a draw.");
			return nullptr;
		default:
			floodWorldState();
		}
	}
	SPDLOG_LOGGER_WARN(spdlog::get("Server"), "Server forced game end.");
	return nullptr;
}

void GameServer::processPackets()
{
	sf::Packet rxPkt;
	std::optional<sf::IpAddress> srcAddrOpt;
	uint16_t srcPort;
	while(checkedReceive(m_gameSock, rxPkt, srcAddrOpt, srcPort) == sf::Socket::Status::Done)
	{
		auto const remote = Endpoint{.ip = *srcAddrOpt, .port = srcPort};
		uint8_t type;
		Tick tick;
		rxPkt >> type >> tick;
		uint32_t clientId = m_lobby.findClient(remote);
		if(!clientId)
			continue;
		if(!m_lobby.m_slots[clientId - 1].bValid || m_lastClientTicks[clientId - 1] >= tick)
		{ // Packet was duplicated or reordered
			SPDLOG_LOGGER_WARN(spdlog::get("Server"), "Outdated pkt from {}, (know {}, got {})", clientId,
			                   m_lastClientTicks[clientId - 1], tick);
			continue;
		}
		m_lastClientTicks[clientId - 1] = tick;

		switch(UnreliablePktType(type))
		{
		case UnreliablePktType::INPUT: {
			WorldUpdateData wud;
			rxPkt >> wud;
			PlayerState &ps = m_world.getPlayerById(clientId);
			if(wud.posDelta != sf::Vector2f{0, 0})
			{
				// m_lobby.m_slots[clientId - 1].bValid &= wud.posDelta.lengthSquared() <= 1;
				ps.moveOn(m_world.m_map, wud.posDelta);
				ps.setCannonRotation(wud.cannonRot);
			}
			if(wud.bShoot && ps.tryShoot())
			{
				ps.setCannonRotation(wud.cannonRot);

				// spawn projectile at cannon tip
				sf::Vector2f tankCenter = ps.getPosition() + sf::Vector2f(PlayerState::logicalDimensions / 2.f);
				sf::Vector2f cannonOffset =
					sf::Vector2f(0.f, -GameConfig::Player::CANNON_LENGTH).rotatedBy(wud.cannonRot);
				sf::Vector2f position = tankCenter + cannonOffset;

				// calculate velocity from cannon angle
				sf::Vector2f velocity = sf::Vector2f(0.f, -GameConfig::Projectile::SPEED).rotatedBy(wud.cannonRot);

				// Apply damage multiplier from powerups
				int damage = GameConfig::Projectile::BASE_DAMAGE * ps.getDamageMultiplier();
				m_world.addProjectile(position, velocity, clientId, damage);
				SPDLOG_LOGGER_INFO(spdlog::get("Server"), "Player {} shoots at t={}, angle={} (damage={})", clientId,
				                   tick, wud.cannonRot.asDegrees(), damage);
			}

			if(wud.slot)
			{
				ps.useItem(wud.slot);
				SPDLOG_LOGGER_INFO(spdlog::get("Server"), "Player {} used item t={}, slot #{}", clientId, tick,
				                   wud.slot);
			}
			break;
		}
		default: {
			std::string const srcAddr = srcAddrOpt.has_value() ? srcAddrOpt->toString() : "?";
			SPDLOG_LOGGER_WARN(spdlog::get("Server"), "Unhandled unreliable packet type: {}, src={}:{}", type, srcAddr,
			                   srcPort);
			break;
		}
		}
	}
}

void GameServer::spawnItems()
{
	if(m_itemSpawnClock.getElapsedTime().asSeconds() > SPAWN_INTERVAL && m_world.getItems().size() < MAX_ITEMS_ON_MAP)
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
	sf::Packet snapPkt = createTickedPkt(UnreliablePktType::SNAPSHOT, m_authTick);
	m_world.serialize(snapPkt);
	snapPkt << m_lastClientTicks;
	for(LobbyPlayer &p : m_lobby.m_slots)
	{
		if(!p.bValid)
			continue;
		if(checkedSend(m_gameSock, snapPkt, p.endpoint.ip, p.endpoint.port) != sf::Socket::Status::Done)
		{
			SPDLOG_LOGGER_ERROR(spdlog::get("Server"), "Failed to send snapshot to {}:{}", p.endpoint.ip.toString(),
			                    p.endpoint.port);
		}
	}
}
