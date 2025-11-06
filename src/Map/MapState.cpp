#include "MapState.h"
#include "Networking.h"

MapState::MapState(sf::Vector2f size)
	: m_spawns(SPAWN_POINTS),
	  m_size(size)
{
	unsigned const width = size.x;
	unsigned const height = size.y;
	float wallThickness = 20.f;

	// Outer and inner walls
	addWall(0, 0, width, wallThickness);
	addWall(0, height - wallThickness, width, wallThickness);
	addWall(0, 0, wallThickness, height);
	addWall(width - wallThickness, 0, wallThickness, height);

	addWall(200, 100, wallThickness, 400); // vertical
	addWall(400, 200, 200, wallThickness); // horizontal
	addWall(600, 50, wallThickness, 300); // vertical
}

void MapState::addWall(float x, float y, float w, float h)
{
	sf::RectangleShape r({w, h});
	r.setPosition({x, y});
	r.setFillColor(sf::Color::Black);
	m_walls.push_back(std::move(r));
}

const std::vector<sf::RectangleShape> &MapState::getWalls() const
{
	return m_walls;
}

std::array<sf::Vector2f, MAX_PLAYERS> MapState::getSpawns() const
{
	return m_spawns;
}

bool MapState::isColliding(const sf::RectangleShape &r) const
{
	sf::FloatRect rbox = r.getGlobalBounds();
	for(auto const &w : m_walls)
	{
		if(rbox.findIntersection(w.getGlobalBounds()))
			return true;
	}
	return false;
}
