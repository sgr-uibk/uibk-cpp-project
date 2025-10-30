#pragma once
#include "Map/MapState.h"
#include <SFML/Graphics.hpp>

class MapClient
{
public:
	explicit MapClient(MapState &state);

	void draw(sf::RenderWindow &window) const;

private:
	MapState &m_state;
};
