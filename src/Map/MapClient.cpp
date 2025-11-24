#include "MapClient.h"

MapClient::MapClient(MapState &state) : m_state(state)
{
}

void MapClient::draw(sf::RenderWindow &window) const
{
	for(auto const &wall : m_state.getWalls())
	{
		if(!wall.isDestroyed())
			window.draw(wall.getShape());
	}
}
