#include "MapClient.h"

MapClient::MapClient(MapState &state) : m_state(state)
{
    for(auto &tile : m_state.getTiles())
    {
        switch(tile.getType())
        {
            case TileType::Wall:  tile.setTexture(TextureManager::inst().load("tiles/wall_brick.png")); break;
            case TileType::Grass: tile.setTexture(TextureManager::inst().load("tiles/grass.png"));      break;
            case TileType::Water: tile.setTexture(TextureManager::inst().load("tiles/water.png"));      break;
            case TileType::Stone: tile.setTexture(TextureManager::inst().load("tiles/stone.png"));      break;
        }
    }
}

void MapClient::draw(sf::RenderWindow &window) const
{
    for(const auto &tile : m_state.getTiles())
        tile.draw(window);
}