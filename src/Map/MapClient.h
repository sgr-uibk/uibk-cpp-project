#pragma once
#include "Map/MapState.h"
#include <SFML/Graphics.hpp>
#include <iostream>
#include "ResourceManager.h"

class MapClient
{
public:
	explicit MapClient(MapState &state);

	void draw(sf::RenderWindow &window) const;

private:
	MapState &m_state;
	std::map<std::string, sf::Texture> m_textures;
};
