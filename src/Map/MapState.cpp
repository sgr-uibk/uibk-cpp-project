#include "MapState.h"
#include "Utilities.h"
#include <spdlog/spdlog.h>

MapState::MapState(sf::Vector2f size) : m_size(size)
{
}

void MapState::addWall(sf::Vector2f pos, sf::Vector2f dim, int health)
{
	m_walls.emplace_back(pos, dim, health);
}

void MapState::addSpawnPoint(sf::Vector2f spawn)
{
	m_spawns.push_back(spawn);
}

void MapState::addItemSpawnZone(sf::Vector2f position, PowerupType itemType)
{
	m_itemSpawnZones.push_back({position, itemType});
}

bool MapState::isColliding(sf::RectangleShape const &r) const
{
	for(auto const &wall : m_walls)
	{
		if(!wall.isDestroyed() && wall.getGlobalBounds().findIntersection(r.getGlobalBounds()).has_value())
		{
			return true;
		}
	}
	return false;
}

std::vector<WallState> const &MapState::getWalls() const
{
	return m_walls;
}

std::vector<WallState> &MapState::getWalls()
{
	return m_walls;
}

std::vector<sf::Vector2f> const &MapState::getSpawns() const
{
	return m_spawns;
}

std::vector<ItemSpawnZone> const &MapState::getItemSpawnZones() const
{
	return m_itemSpawnZones;
}

sf::Vector2f MapState::getSize() const
{
	return m_size;
}

void MapState::loadFromBlueprint(MapBlueprint const &bp)
{
	m_size = bp.getTotalSize();

	m_tileset = bp.tileset;
	m_layers = bp.layers;

	m_walls.clear();
	m_spawns.clear();
	m_itemSpawnZones.clear();

	for(auto const &layer : bp.layers)
	{
		if(layer.name != "Walls")
			continue;

		sf::Vector2i pos = {0, 0};
		for(; pos.y < layer.dim.y; ++pos.y)
		{
			for(pos.x = 0; pos.x < layer.dim.x; ++pos.x)
			{
				int idx = pos.y * layer.dim.x + pos.x;
				int tileId = layer.data[idx];

				if(tileId != 0)
				{
					sf::Vector2f world = sf::Vector2f(pos) * CARTESIAN_TILE_SIZE;
					m_walls.emplace_back(world, sf::Vector2f{CARTESIAN_TILE_SIZE, CARTESIAN_TILE_SIZE}, 100);
				}
			}
		}
	}

	for(auto const &obj : bp.objects)
	{
		if(obj.type == "player_spawn")
		{
			addSpawnPoint(obj.position);

			spdlog::info("Added player spawn at ({}, {})", obj.position.x, obj.position.y);
		}
		else if(obj.type == "item_spawn")
		{
			std::string itemTypeStr = obj.getProperty("item_type", "HEALTH_PACK");
			PowerupType type = stringToPowerupType(itemTypeStr);

			addItemSpawnZone(obj.position, type);

			spdlog::info("Added item spawn {} at ({}, {})", itemTypeStr, obj.position.x, obj.position.y);
		}
	}

	spdlog::info("Map Built: {} walls, {} spawns", m_walls.size(), m_spawns.size());
}

PowerupType MapState::stringToPowerupType(std::string const &str)
{
	if(str == "HEALTH_PACK")
		return PowerupType::HEALTH_PACK;
	if(str == "SPEED_BOOST")
		return PowerupType::SPEED_BOOST;
	if(str == "DAMAGE_BOOST")
		return PowerupType::DAMAGE_BOOST;
	if(str == "SHIELD")
		return PowerupType::SHIELD;
	if(str == "RAPID_FIRE")
		return PowerupType::RAPID_FIRE;
	return PowerupType::HEALTH_PACK;
}

std::optional<RawLayer> MapState::getGroundLayer() const
{
	for(auto const &layer : m_layers)
	{
		if(layer.name == "Ground")
		{
			return layer;
		}
	}
	return std::nullopt;
}

std::optional<RawLayer> MapState::getWallsLayer() const
{
	for(auto const &layer : m_layers)
	{
		if(layer.name == "Walls")
		{
			return layer;
		}
	}
	return std::nullopt;
}

std::vector<WallState> const &MapState::getWallStates() const
{
	return m_walls;
}

std::optional<RawTileset> const &MapState::getTileset() const
{
	return m_tileset;
}

std::vector<RawLayer> const &MapState::getLayers() const
{
	return m_layers;
}

void MapState::setTileset(std::optional<RawTileset> const &tileset)
{
	m_tileset = tileset;
}

void MapState::setGroundLayer(std::optional<RawLayer> const &layer)
{
	if(!layer.has_value())
		return;

	for(auto &l : m_layers)
	{
		if(l.name == "Ground")
		{
			l = *layer;
			return;
		}
	}
	m_layers.push_back(*layer);
}

void MapState::setWallsLayer(std::optional<RawLayer> const &layer)
{
	if(!layer.has_value())
		return;

	for(auto &l : m_layers)
	{
		if(l.name == "Walls")
		{
			l = *layer;
			return;
		}
	}
	m_layers.push_back(*layer);
}

WallState const *MapState::getWallAtGridPos(sf::Vector2i const pos) const
{
	auto const cellCenter = sf::Vector2f(pos) * CARTESIAN_TILE_SIZE + sf::Vector2f{1, 1} * (CARTESIAN_TILE_SIZE / 2.0f);

	for(auto const &wall : m_walls)
	{
		sf::FloatRect const bounds = wall.getGlobalBounds();
		sf::Vector2f const wallCenter = bounds.position + bounds.size / 2.f;
		sf::Vector2f diff = cellCenter - wallCenter;
		diff = diff.componentWiseMul(diff);
		if(diff.x < 1.f && diff.y < 1.f)
			return &wall;
	}

	return nullptr;
}

void MapState::destroyWallAtGridPos(sf::Vector2i pos)
{
	for(auto &layer : m_layers)
	{
		if(layer.name == "Walls")
		{
			size_t const idx = pos.y * layer.dim.x + pos.x;
			if(idx < layer.data.size())
			{
				layer.data[idx] = 0; // Set to empty tile
				spdlog::debug("Tile swap: Cleared wall tile at grid ({}, {})", pos.x, pos.y);
			}
			break;
		}
	}
}
