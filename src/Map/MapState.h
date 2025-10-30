#pragma once
#include <vector>
#include <SFML/Graphics.hpp>

class MapState
{
public:
	explicit MapState(sf::Vector2f size);

	void addWall(float x, float y, float w, float h);
	[[nodiscard]] bool isColliding(const sf::RectangleShape &r) const;
	[[nodiscard]] const std::vector<sf::RectangleShape> &getWalls() const;

private:
	sf::Vector2f m_size;
	std::vector<sf::RectangleShape> m_walls;
};
