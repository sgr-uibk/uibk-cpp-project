#pragma once
#include "Map/MapState.h"
#include <SFML/Graphics.hpp>

class MapClient
{
public:
	explicit MapClient(const MapState &state);

	void setMapState(const MapState &state);
	void draw(sf::RenderWindow &window) const;

private:
	MapState m_state;
};
