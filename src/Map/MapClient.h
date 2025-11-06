#pragma once
#include "Map/MapState.h"
#include <SFML/Graphics.hpp>

class MapClient
{
public:
	explicit MapClient(MapState const &state);

	void draw(sf::RenderWindow &window) const;

private:
	MapState const &m_state;
};
