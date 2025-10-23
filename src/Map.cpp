#include "Map.h"

Map::Map(float width, float height, float wall_thickness)
{
	auto addWall = [&](float x, float y, float w, float h) {
		sf::RectangleShape r({w, h});
		r.setPosition({x, y});
		r.setFillColor(sf::Color::Black);
		m_walls.push_back(r);
	};

	// Outer and inner walls
	addWall(0, 0, width, wall_thickness);
	addWall(0, height - wall_thickness, width, wall_thickness);
	addWall(0, 0, wall_thickness, height);
	addWall(width - wall_thickness, 0, wall_thickness, height);

	addWall(200, 100, wall_thickness, 400); // vertical
	addWall(400, 200, 200, wall_thickness); // horizontal
	addWall(600, 50, wall_thickness, 300); // vertical
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
