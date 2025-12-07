#pragma once
#include <string>
#include <vector>
#include <map>
#include <optional>
#include <SFML/System/Vector2.hpp>
#include "Utilities.h"

// =============================================================================
// MAP BLUEPRINT (DTOs)
// =============================================================================

struct RawTileset
{
	std::string imagePath;
	int tileWidth;
	int tileHeight;
	int columns;
	int firstGid;
	int spacing = 0;
	int margin = 0;
	int mapTileWidth;
	int mapTileHeight;
};

struct RawLayer
{
	std::string name;
	int width;
	int height;
	std::vector<int> data;
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

	std::string getProperty(const std::string &key, const std::string &defaultVal = "") const
	{
		auto it = properties.find(key);
		return (it != properties.end()) ? it->second : defaultVal;
	}
};

struct MapBlueprint
{
	int widthInTiles;
	int heightInTiles;
	int tileWidth;
	int tileHeight;

	std::optional<RawTileset> tileset;

	std::vector<RawLayer> layers;
	std::vector<RawObject> objects;

	sf::Vector2f getTotalSize() const
	{
		return {static_cast<float>(widthInTiles) * CARTESIAN_TILE_SIZE,
		        static_cast<float>(heightInTiles) * CARTESIAN_TILE_SIZE};
	}
};