#include "MapState.h"
#include "Utilities.h"
#include <spdlog/spdlog.h>

MapState::MapState(sf::Vector2f size) : m_size(size)
{
}

RawLayer *MapState::findLayerByName(std::string const &name)
{
	for(auto &layer : m_layers)
	{
		if(layer.name == name)
			return &layer;
	}
	return nullptr;
}

RawLayer const *MapState::findLayerByName(std::string const &name) const
{
	for(auto const &layer : m_layers)
	{
		if(layer.name == name)
			return &layer;
	}
	return nullptr;
}

void MapState::addSpawnPoint(sf::Vector2f spawn)
{
	m_spawns.push_back(spawn);
}

void MapState::addItemSpawnZone(sf::Vector2f position, PowerupType itemType)
{
	m_itemSpawnZones.push_back({position, itemType});
}

bool MapState::isRendered(WallState const &wall, RawLayer const &layer)
{
	sf::Vector2i const pos = sf::Vector2i{wall.getShape().getPosition() / CARTESIAN_TILE_SIZE};
	size_t const idx = pos.y * layer.dim.x + pos.x;
	return layer.data[idx];
}

bool MapState::isColliding(sf::RectangleShape const &r) const
{
	auto wallsLayerOpt = getWallsLayer();
	assert(wallsLayerOpt.has_value());

	for(auto const &wall : m_walls)
	{
		bool const bRenderWallExists = isRendered(wall, *wallsLayerOpt);
		if(bRenderWallExists && wall.getGlobalBounds().findIntersection(r.getGlobalBounds()).has_value())
			return true;
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

	for(auto const &layer : m_layers)
	{
		if(layer.name != LayerName::WALLS)
			continue;

		sf::Vector2i pos = {0, 0};
		for(; pos.y < layer.dim.y; ++pos.y)
		{
			for(pos.x = 0; pos.x < layer.dim.x; ++pos.x)
			{
				int const idx = pos.y * layer.dim.x + pos.x;
				TileType const tileType = layer.data[idx];

				int health = BASE_WALL_HEALTH;
				switch(tileType)
				{
				default:
				case AIR:
					break;
				case REINFORCED_WALL:
					health *= REINFORCED_HEALTH_MULTIPLIER;
					[[fallthrough]];
				case WALL:
					sf::Vector2f world = sf::Vector2f(pos) * CARTESIAN_TILE_SIZE;
					m_walls.emplace_back(world, sf::Vector2f{CARTESIAN_TILE_SIZE, CARTESIAN_TILE_SIZE}, health);
					break;
				}
			}
		}
	}

	for(auto const &obj : bp.objects)
	{
		if(obj.type == ObjectType::PLAYER_SPAWN)
		{
			addSpawnPoint(obj.position);
			spdlog::info("Added player spawn at ({}, {})", obj.position.x, obj.position.y);
		}
		else if(obj.type == ObjectType::ITEM_SPAWN)
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
	auto const *layer = findLayerByName(LayerName::GROUND);
	if(layer)
		return *layer;
	return std::nullopt;
}

std::optional<RawLayer> MapState::getWallsLayer() const
{
	auto const *layer = findLayerByName(LayerName::WALLS);
	if(layer)
		return *layer;
	return std::nullopt;
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

	auto *existing = findLayerByName(LayerName::GROUND);
	if(existing)
		*existing = *layer;
	else
		m_layers.push_back(*layer);
}

void MapState::setWallsLayer(std::optional<RawLayer> const &layer)
{
	if(!layer.has_value())
		return;

	auto *existing = findLayerByName(LayerName::WALLS);
	if(existing)
		*existing = *layer;
	else
		m_layers.push_back(*layer);
}

WallState const *MapState::getWallAtGridPos(sf::Vector2i const pos) const
{
	sf::Vector2f const halfTile{CARTESIAN_TILE_SIZE / 2.0f, CARTESIAN_TILE_SIZE / 2.0f};
	sf::Vector2f const cellCenter = sf::Vector2f(pos) * CARTESIAN_TILE_SIZE + halfTile;

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
	auto *wallsLayer = findLayerByName(LayerName::WALLS);
	if(!wallsLayer)
		return;

	size_t const idx = pos.y * wallsLayer->dim.x + pos.x;
	if(idx < wallsLayer->data.size())
	{
		wallsLayer->data[idx] = AIR;
		spdlog::debug("Tile swap: Cleared wall tile at grid ({}, {})", pos.x, pos.y);
	}
}
