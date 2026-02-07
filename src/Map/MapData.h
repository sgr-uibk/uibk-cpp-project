#pragma once
#include <string>
#include <vector>
#include <map>
#include <optional>
#include <SFML/System/Vector2.hpp>
#include "../GameConfig.h"

// =============================================================================
// MAP BLUEPRINT (DTOs)
// =============================================================================

// Layer name constants used by Tiled export
namespace LayerName
{
constexpr char const *GROUND = "Ground";
constexpr char const *WALLS = "Walls";
} // namespace LayerName

// Object type constants used by Tiled export
namespace ObjectType
{
constexpr char const *PLAYER_SPAWN = "player_spawn";
constexpr char const *ITEM_SPAWN = "item_spawn";
} // namespace ObjectType

struct RawTileset
{
	std::string imagePath;
	sf::Vector2i tileDim;
	sf::Vector2i mapTileDim;
	int columns;
	int spacing = 0;
	int margin = 0;
};

// Sprite indices in the single map tileset spritesheet (assets/map)
enum TileType : int
{
	AIR = 0,
	WALL = 1,
	REINFORCED_WALL = 2,
	BRICK_FLOOR = 3
};

struct RawLayer
{
	std::string name;
	sf::Vector2i dim;
	std::vector<TileType> data;
	bool visible = true;
};

struct RawObject
{
	int id;
	std::string name;
	std::string type; // e.g., "player_spawn", "item"
	sf::Vector2f position;
	sf::Vector2f size;
	float rotation = 0.f;

	// Custom properties defined in Tiled (e.g., "health", "item_type")
	std::map<std::string, std::string> properties;

	std::string getProperty(std::string const &key, std::string const &defaultVal = "") const
	{
		auto it = properties.find(key);
		return (it != properties.end()) ? it->second : defaultVal;
	}
};

struct MapBlueprint
{
	sf::Vector2i dimInTiles;
	sf::Vector2i tileDim;
	std::optional<RawTileset> tileset;
	std::vector<RawLayer> layers;
	std::vector<RawObject> objects;

	[[nodiscard]] sf::Vector2f getTotalSize() const
	{
		return sf::Vector2f(dimInTiles) * CARTESIAN_TILE_SIZE;
	}
};
