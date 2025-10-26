#include "Map.h"

Map::Map(sf::Vector2u dimensions, float wallThickness)
{
	auto addWall = [&](float x, float y, float w, float h) {
		sf::RectangleShape r({w, h});
		r.setPosition({x, y});
		r.setFillColor(sf::Color::Black);
		m_walls.push_back(r);
	};
	unsigned const width = dimensions.x;
	unsigned const height = dimensions.y;

	// Outer and inner walls
	addWall(0, 0, width, wallThickness);
	addWall(0, height - wallThickness, width, wallThickness);
	addWall(0, 0, wallThickness, height);
	addWall(width - wallThickness, 0, wallThickness, height);

	addWall(200, 100, wallThickness, 400); // vertical
	addWall(400, 200, 200, wallThickness); // horizontal
	addWall(600, 50, wallThickness, 300); // vertical
}

bool Map::isCollidingWithWalls(sf::FloatRect const &it) const
{
	bool collision = false;
	for(auto &wall : m_walls)
	{
		if(it.findIntersection(wall.getGlobalBounds()) != std::nullopt)
		{
			collision = true;
			break;
		}
	}
	return collision;
}

const std::vector<sf::RectangleShape> & Map::getWalls() const
{
	return m_walls;
}

void Map::draw(sf::RenderWindow &window) const
{
	for(const auto &wall : m_walls)
		window.draw(wall);
}
