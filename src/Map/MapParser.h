#pragma once
#include "MapData.h"
#include <string>
#include <optional>

class MapParser
{
  public:
	static std::optional<MapBlueprint> parse(const std::string &filePath);
};