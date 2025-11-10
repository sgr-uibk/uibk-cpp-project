#include "MapLoader.h"
#include <fstream>
#include <iostream>

bool MapLoader::loadMap(MapState &state, const std::string &path, sf::Vector2f mapSize, float tileSize)
{
    state.reset(mapSize);
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open map file: " << path << "\n";
        return false;
    }

    std::string line;
    unsigned row = 0;
    while (std::getline(file, line)) {
        for (unsigned col = 0; col < line.size(); ++col) {
            char c = line[col];
            TileType type;

            switch(c) {
                case '#': type = TileType::Wall; break;
                case 'g': type = TileType::Grass; break;
                case 'w': type = TileType::Water; break;
                case 's': type = TileType::Stone; break;
                default:  type = TileType::Grass; break;
            }

            if(c >= '1' && c <= '4') {
                int playerId = c - '0';
                state.setPlayerSpawn(playerId, {col * tileSize, row * tileSize});
            } else if (c == '#' || c == 'w') {
                state.addWall(col * tileSize, row * tileSize, tileSize, tileSize);
            }
        }
        ++row;
    }

    return true;
}