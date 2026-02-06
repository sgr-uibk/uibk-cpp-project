#include <algorithm>
#include <utility>
#include <cmath>
#include "Map/MapParser.h"
#include <spdlog/spdlog.h>
#include "Utilities.h"
#include "WorldState.h"
#include "GameConfig.h"

WorldState::WorldState(sf::Packet &pkt)
	: m_map(WINDOW_DIMf),
	  m_players(deserializeToArray<PlayerState, MAX_PLAYERS>(pkt, std::make_index_sequence<MAX_PLAYERS>{}))
{
	deserialize(pkt);
}

WorldState::WorldState(int mapIndex, std::array<PlayerState, MAX_PLAYERS> playersInit)
	: m_map(sf::Vector2f(0, 0)), m_players(std::move(playersInit))
{
	std::string mapPath = Maps::MAP_PATHS[mapIndex];
	auto blueprint = MapParser::parse(mapPath);

	if(!blueprint)
	{
		spdlog::error("Failed to load map from index {} (path: {})", mapIndex, mapPath);
		m_map = MapState(WINDOW_DIMf);
		return;
	}

	m_map.loadFromBlueprint(*blueprint);
	spdlog::info("Loaded map from index {} (path: {})", mapIndex, mapPath);

	sf::Vector2f const mapSize = m_map.getSize();
	m_safeZone = {.center = mapSize / 2.f,
	              .currentRadius = std::max(mapSize.x, mapSize.y) * 0.75f,
	              .targetRadius = GameConfig::SafeZone::MIN_RADIUS,
	              .shrinkSpeed = GameConfig::SafeZone::SHRINK_SPEED,
	              .damagePerSecond = GameConfig::SafeZone::DAMAGE_PER_SECOND,
	              .isActive = false};
}

WorldState::WorldState(sf::Vector2f const mapSize, std::array<PlayerState, MAX_PLAYERS> playersInit)
	: m_map(mapSize), m_players(std::move(playersInit))
{
}

void WorldState::update(float const dt)
{
	for(auto &p : m_players)
		p.update(dt);
	for(auto &proj : m_projectiles)
		proj.update(dt);
	for(auto &item : m_items)
		item.update(dt);

	m_safeZone.update(dt);
	if(!m_safeZone.isActive)
		return;

	for(auto &p : m_players)
	{
		if(p.m_id == 0 || !p.isAlive())
			continue;

		sf::Vector2f const playerCenter = p.getPosition() + PlayerState::logicalDimensions / 2.f;
		if(!m_safeZone.isOutside(playerCenter))
		{
			p.m_zoneDamageAccum = 0.f;
			continue;
		}

		p.m_zoneDamageAccum += m_safeZone.damagePerSecond * dt;
		if(p.m_zoneDamageAccum >= 1.f)
		{
			int const damage = static_cast<int>(p.m_zoneDamageAccum);
			p.takeDamage(damage);
			p.m_zoneDamageAccum -= static_cast<float>(damage);
		}
	}
}

void WorldState::setPlayer(PlayerState const &p)
{
	size_t const idx = p.m_id - 1;
	m_players[idx] = p;
}

std::array<PlayerState, MAX_PLAYERS> &WorldState::getPlayers()
{
	return m_players;
}

PlayerState &WorldState::getPlayerById(size_t id)
{
	if(id == 0)
		SPDLOG_ERROR("ID 0 is reserved for the server!");
	return m_players[id - 1];
}

PlayerState const &WorldState::getPlayerById(size_t id) const
{
	if(id == 0)
		SPDLOG_ERROR("ID 0 is reserved for the server!");
	return m_players[id - 1];
}

MapState &WorldState::getMap()
{
	return m_map;
}

MapState const &WorldState::getMap() const
{
	return m_map;
}

uint32_t WorldState::addProjectile(sf::Vector2f position, sf::Vector2f velocity, uint32_t ownerId, int damage)
{
	uint32_t id = m_nextProjectileId++;
	m_projectiles.emplace_back(id, position, velocity, ownerId, damage);
	return id;
}

void WorldState::removeProjectile(uint32_t id)
{
	auto it = std::find_if(m_projectiles.begin(), m_projectiles.end(),
	                       [id](ProjectileState const &p) { return p.getId() == id; });
	if(it != m_projectiles.end())
	{
		m_projectiles.erase(it);
	}
}

void WorldState::removeInactiveProjectiles()
{
	m_projectiles.erase(std::remove_if(m_projectiles.begin(), m_projectiles.end(),
	                                   [](ProjectileState const &p) { return !p.isActive(); }),
	                    m_projectiles.end());
}

void WorldState::checkProjectilePlayerCollisions()
{
	for(auto &proj : m_projectiles)
	{
		if(!proj.isActive())
			continue;

		sf::FloatRect projBounds = proj.getBounds();

		for(auto &player : m_players)
		{
			if(!player.isAlive())
				continue;

			if(player.getPlayerId() == proj.getOwnerId())
				continue;

			sf::FloatRect playerBounds(player.getPosition(), PlayerState::logicalDimensions);

			if(projBounds.findIntersection(playerBounds).has_value())
			{
				player.takeDamage(proj.getDamage());
				proj.deactivate();
				break;
			}
		}
	}
}

void WorldState::checkProjectileWallCollisions()
{
	for(auto &proj : m_projectiles)
	{
		if(!proj.isActive())
			continue;

		sf::FloatRect projBounds = proj.getBounds();
		auto &walls = m_map.getWalls();

		for(auto &wall : walls)
		{
			if(wall.isDestroyed())
				continue;

			if(projBounds.findIntersection(wall.getGlobalBounds()))
			{
				wall.takeDamage(proj.getDamage());

				if(wall.isDestroyed())
				{
					auto pos = wall.getShape().getPosition();
					sf::Vector2i grid = sf::Vector2i{pos / CARTESIAN_TILE_SIZE};
					m_map.destroyWallAtGridPos(grid);
					markWallDestroyed(grid);
				}

				proj.deactivate();
				break;
			}
		}
	}
}

uint32_t WorldState::addItem(sf::Vector2f position, PowerupType type)
{
	uint32_t id = m_nextItemId++;
	m_items.emplace_back(id, position, type);
	return id;
}

void WorldState::removeItem(uint32_t id)
{
	auto it = std::find_if(m_items.begin(), m_items.end(), [id](ItemState const &i) { return i.getId() == id; });
	if(it != m_items.end())
	{
		m_items.erase(it);
	}
}

void WorldState::removeInactiveItems()
{
	m_items.erase(std::remove_if(m_items.begin(), m_items.end(), [](ItemState const &i) { return !i.isActive(); }),
	              m_items.end());
}

void WorldState::checkPlayerItemCollisions()
{
	for(auto &item : m_items)
	{
		if(!item.isActive())
			continue;

		sf::FloatRect itemBounds = item.getBounds();

		for(auto &player : m_players)
		{
			if(!player.isAlive())
				continue;

			sf::FloatRect playerBounds(player.getPosition(), PlayerState::logicalDimensions);

			if(itemBounds.findIntersection(playerBounds).has_value())
			{
				if(player.addToInventory(item.getType()))
				{
					item.collect();
				}
				break;
			}
		}
	}
}

void WorldState::checkPlayerPlayerCollisions()
{
	for(size_t i = 0; i < m_players.size(); ++i)
	{
		if(!m_players[i].isAlive() || m_players[i].m_id == 0)
			continue;

		sf::FloatRect bounds1(m_players[i].getPosition(), PlayerState::logicalDimensions);

		for(size_t j = i + 1; j < m_players.size(); ++j)
		{
			if(!m_players[j].isAlive() || m_players[j].m_id == 0)
				continue;

			sf::FloatRect bounds2(m_players[j].getPosition(), PlayerState::logicalDimensions);

			if(bounds1.findIntersection(bounds2).has_value())
			{
				// players colliding -> push them apart
				sf::Vector2f pos1 = m_players[i].getPosition();
				sf::Vector2f pos2 = m_players[j].getPosition();

				sf::Vector2f diff = pos1 - pos2;
				float distance = std::sqrt(diff.x * diff.x + diff.y * diff.y);

				if(distance < 0.01f)
				{
					diff = sf::Vector2f(1.0f, 0.0f);
					distance = 1.0f;
				}

				sf::Vector2f pushDir = diff / distance;

				float overlap =
					(PlayerState::logicalDimensions.x / 2.0f + PlayerState::logicalDimensions.x / 2.0f) - distance;

				float pushAmount = overlap / 2.0f + 0.5f;
				m_players[i].m_iState.m_pos += pushDir * pushAmount;
				m_players[j].m_iState.m_pos -= pushDir * pushAmount;

				sf::RectangleShape shape1(PlayerState::logicalDimensions);
				shape1.setPosition(m_players[i].getPosition());
				if(m_map.isColliding(shape1))
				{
					m_players[i].m_iState.m_pos -= pushDir * pushAmount;
				}

				sf::RectangleShape shape2(PlayerState::logicalDimensions);
				shape2.setPosition(m_players[j].getPosition());
				if(m_map.isColliding(shape2))
				{
					m_players[j].m_iState.m_pos += pushDir * pushAmount;
				}
			}
		}
	}
}

void WorldState::serialize(sf::Packet &pkt) const
{
	serializeFromArray(pkt, m_players, std::make_index_sequence<MAX_PLAYERS>{});

	uint32_t const numProjectiles = static_cast<uint32_t>(m_projectiles.size());
	pkt << numProjectiles;
	for(auto const &proj : m_projectiles)
	{
		proj.serialize(pkt);
	}

	uint32_t const numItems = static_cast<uint32_t>(m_items.size());
	pkt << numItems;
	for(auto const &item : m_items)
	{
		item.serialize(pkt);
	}

	// delta only
	uint32_t const numDestroyedWalls = static_cast<uint32_t>(m_destroyedWallsThisTick.size());
	pkt << numDestroyedWalls;
	for(auto const &grid : m_destroyedWallsThisTick)
		pkt << grid;

	pkt << m_safeZone;
}

void WorldState::assignSnappedState(WorldState const &other)
{
	this->m_items = other.m_items;
	this->m_nextItemId = other.m_nextItemId;
	this->m_nextProjectileId = other.m_nextProjectileId;
	this->m_projectiles = other.m_projectiles;
	this->m_safeZone = other.m_safeZone;

	for(size_t i = 0; i < m_players.size(); ++i)
	{
		this->m_players[i].assignSnappedState(other.m_players[i]);
	}
}

void WorldState::deserialize(sf::Packet &pkt)
{

	uint32_t numProjectiles = 0;
	pkt >> numProjectiles;
	m_projectiles.clear();
	m_projectiles.reserve(numProjectiles);
	for(uint32_t i = 0; i < numProjectiles; ++i)
	{
		ProjectileState proj;
		proj.deserialize(pkt);
		m_projectiles.push_back(proj);
	}

	uint32_t numItems = 0;
	pkt >> numItems;
	m_items.clear();
	m_items.reserve(numItems);
	for(uint32_t i = 0; i < numItems; ++i)
	{
		ItemState item;
		item.deserialize(pkt);
		m_items.push_back(item);
	}

	uint32_t numDestroyedWalls = 0;
	pkt >> numDestroyedWalls;

	m_destroyedWallsThisTick.clear();

	for(uint32_t i = 0; i < numDestroyedWalls; ++i)
	{
		sf::Vector2i gridPos;
		pkt >> gridPos;

		m_destroyedWallsThisTick.push_back(gridPos);
	}

	pkt >> m_safeZone;
}

void WorldState::clearWallDeltas()
{
	m_destroyedWallsThisTick.clear();
}

void WorldState::markWallDestroyed(sf::Vector2i gridPos)
{
	m_destroyedWallsThisTick.push_back(gridPos);
	spdlog::debug("Marked wall at grid ({}, {}) for delta update", gridPos.x, gridPos.y);
}

std::vector<sf::Vector2i> const &WorldState::getDestroyedWallDeltas() const
{
	return m_destroyedWallsThisTick;
}
