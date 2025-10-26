#pragma once
#include <SFML/Graphics.hpp>
#include <vector>

// this is very rudimentary, I need it to get the Player class running.
class Map {
	std::vector<sf::RectangleShape> m_walls;

public:
	Map(sf::Vector2u dimensions, float wallThickness = 20.f);
	bool isCollidingWithWalls(sf::FloatRect const& it) const;
	const std::vector<sf::RectangleShape>& getWalls() const;
	void draw(sf::RenderWindow& window) const;
};
