#include <algorithm>
#include <cmath>
#include "WorldState.h"
#include "Map/MapParser.h"

#include <spdlog/spdlog.h>

WorldState::WorldState() : m_map(WINDOW_DIMf), m_players()
{
}

WorldState::WorldState(sf::Vector2f mapSize) : m_map(mapSize), m_players()
{
}

WorldState WorldState::fromTiledMap(const std::string &tiledJsonPath)
{
	WorldState world(sf::Vector2f(0, 0));

	auto blueprint = MapParser::parse(tiledJsonPath);

	if(!blueprint)
	{
		spdlog::error("Failed to load map from {}", tiledJsonPath);
		return world;
	}

	world.m_map.loadFromBlueprint(*blueprint);

	return world;
}

void WorldState::update(float dt)
{
	for(auto &p : m_players)
		p.update(dt);

	for(auto &proj : m_projectiles)
		proj.update(dt);

	for(auto &item : m_items)
		item.update(dt);
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

const PlayerState &WorldState::getPlayerById(size_t id) const
{
	if(id == 0)
		SPDLOG_ERROR("ID 0 is reserved for the server!");
	return m_players[id - 1];
}

MapState &WorldState::getMap()
{
	return m_map;
}

const MapState &WorldState::getMap() const
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
	                       [id](const ProjectileState &p) { return p.getId() == id; });
	if(it != m_projectiles.end())
	{
		m_projectiles.erase(it);
	}
}

void WorldState::removeInactiveProjectiles()
{
	m_projectiles.erase(std::remove_if(m_projectiles.begin(), m_projectiles.end(),
	                                   [](const ProjectileState &p) { return !p.isActive(); }),
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
				bool wasAlive = player.isAlive();
				player.takeDamage(proj.getDamage());

				if(wasAlive && !player.isAlive())
				{
					player.incrementDeaths();

					for(auto &shooter : m_players)
					{
						if(shooter.getPlayerId() == proj.getOwnerId())
						{
							shooter.incrementKills();
							break;
						}
					}
				}

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
					int gridX = static_cast<int>(pos.x / CARTESIAN_TILE_SIZE);
					int gridY = static_cast<int>(pos.y / CARTESIAN_TILE_SIZE);
					m_map.destroyWallAtGridPos(gridX, gridY);

					markWallDestroyed(gridX, gridY);
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
	auto it = std::find_if(m_items.begin(), m_items.end(), [id](const ItemState &i) { return i.getId() == id; });
	if(it != m_items.end())
	{
		m_items.erase(it);
	}
}

void WorldState::removeInactiveItems()
{
	m_items.erase(std::remove_if(m_items.begin(), m_items.end(), [](const ItemState &i) { return !i.isActive(); }),
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
				m_players[i].m_pos += pushDir * pushAmount;
				m_players[j].m_pos -= pushDir * pushAmount;

				sf::RectangleShape shape1(PlayerState::logicalDimensions);
				shape1.setPosition(m_players[i].getPosition());
				if(m_map.isColliding(shape1))
				{
					m_players[i].m_pos -= pushDir * pushAmount;
				}

				sf::RectangleShape shape2(PlayerState::logicalDimensions);
				shape2.setPosition(m_players[j].getPosition());
				if(m_map.isColliding(shape2))
				{
					m_players[j].m_pos += pushDir * pushAmount;
				}
			}
		}
	}
}

void WorldState::serialize(sf::Packet &pkt) const
{
	for(uint32_t i = 0; i < MAX_PLAYERS; ++i)
	{
		m_players[i].serialize(pkt);
	}

	uint32_t numProjectiles = static_cast<uint32_t>(m_projectiles.size());
	pkt << numProjectiles;
	for(const auto &proj : m_projectiles)
	{
		proj.serialize(pkt);
	}

	uint32_t numItems = static_cast<uint32_t>(m_items.size());
	pkt << numItems;
	for(const auto &item : m_items)
	{
		item.serialize(pkt);
	}

	// delta only: send only walls destroyed this tick
	uint32_t numDestroyedWalls = static_cast<uint32_t>(m_destroyedWallsThisTick.size());
	pkt << numDestroyedWalls;
	for(const auto &[gridX, gridY] : m_destroyedWallsThisTick)
	{
		pkt << gridX;
		pkt << gridY;
	}
}

void WorldState::deserialize(sf::Packet &pkt)
{
	for(uint32_t i = 0; i < MAX_PLAYERS; ++i)
	{
		m_players[i].deserialize(pkt);
	}

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
		int32_t gridX, gridY;
		pkt >> gridX >> gridY;

		m_destroyedWallsThisTick.push_back({gridX, gridY});
	}
}

// The map is static, it's not serialized in snapshots, therefore don't assign it.
// Wall deltas are sent separately and applied via applyWallDeltas()
WorldState &WorldState::operator=(const WorldState &other)
{
	if(this != &other)
	{
		this->m_players = other.m_players;
		this->m_projectiles = other.m_projectiles;
		this->m_items = other.m_items;
		this->m_nextProjectileId = other.m_nextProjectileId;
		this->m_nextItemId = other.m_nextItemId;
	}
	return *this;
}

void WorldState::clearWallDeltas()
{
	m_destroyedWallsThisTick.clear();
}

void WorldState::markWallDestroyed(int gridX, int gridY)
{
	m_destroyedWallsThisTick.push_back({gridX, gridY});
	spdlog::debug("Marked wall at grid ({}, {}) for delta update", gridX, gridY);
}

const std::vector<std::pair<int, int>> &WorldState::getDestroyedWallDeltas() const
{
	return m_destroyedWallsThisTick;
}
