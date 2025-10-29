#include "MapClient.h"

MapClient::MapClient(const MapState &state)
{
	setMapState(state);
}

void MapClient::setMapState(const MapState &state)
{
	m_state = state;
}

void MapClient::draw(sf::RenderWindow &window) const
{
	for(auto const &s : m_state.getWalls())
		window.draw(s);
}
