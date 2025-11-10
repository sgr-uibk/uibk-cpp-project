#pragma once
#include <string>
#include "MapState.h"

class MapLoader {
public:
    static bool loadMap(MapState &state, const std::string &path, sf::Vector2f mapSize, float tileSize = 48.f);
};