#pragma once
#include <vector>
#include <SFML/Graphics.hpp>
#include "WallState.h"

class MapState
{
  public:
	explicit MapState(sf::Vector2f size);

	void addWall(float x, float y, float w, float h, int health = 100);
	void addSpawnPoint(sf::Vector2f spawn);
	[[nodiscard]] bool isColliding(const sf::RectangleShape &r) const;
	[[nodiscard]] const std::vector<WallState> &getWalls() const;
	[[nodiscard]] std::vector<WallState> &getWalls();
	[[nodiscard]] const std::vector<sf::Vector2f> &getSpawns() const;

  private:
	sf::Vector2f m_size;
	std::vector<WallState> m_walls;
	std::vector<sf::Vector2f> m_spawns;
};
