#pragma once
#include <SFML/Graphics.hpp>
#include <vector>

// this is very rudimentary, I need it to get the Player class running.
class Map {
	std::vector<sf::RectangleShape> m_walls;

public:
	Map(float width, float height, float wall_thickness = 20.f);
	bool isCollidingWithWalls(sf::FloatRect const& it) const;
	const std::vector<sf::RectangleShape>& getWalls() const;
	void draw(sf::RenderWindow& window) const;
};
