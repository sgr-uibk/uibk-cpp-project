#pragma once
#include "Map/MapState.h"
#include <SFML/Graphics.hpp>
#include <iostream>
#include "ResourceManager.h"
#include <fstream>

class MapClient
{
public:
	MapClient(MapState &state, const std::string& mapFile, float tileSize);

	void draw(sf::RenderWindow &window) const;

private:
	MapState &m_state;
	std::vector<Tile> m_tiles;
	std::map<std::string, sf::Texture> m_textures;
};
