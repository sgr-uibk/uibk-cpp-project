#pragma once
#include <vector>
#include <SFML/Graphics.hpp>
#include "Networking.h"

class MapState
{
  public:
	explicit MapState(sf::Vector2f size);

	void addWall(float x, float y, float w, float h);
	[[nodiscard]] bool isColliding(const sf::RectangleShape &r) const;
	[[nodiscard]] const std::vector<sf::RectangleShape> &getWalls() const;
	[[nodiscard]] std::array<sf::Vector2f, MAX_PLAYERS> getSpawns() const;

  private:
	std::array<sf::Vector2f, MAX_PLAYERS> m_spawns;
	sf::Vector2f m_size;
	std::vector<sf::RectangleShape> m_walls;
};
