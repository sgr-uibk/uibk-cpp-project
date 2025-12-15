#pragma once
#include "MapData.h"
#include <string>
#include <optional>
#include <vector>
#include <SFML/System/Vector2.hpp>

class MapParser
{
  public:
	static std::optional<MapBlueprint> parse(std::string const &filePath);
	static std::vector<sf::Vector2f> parseSpawnsOnly(std::string const &filePath);
};