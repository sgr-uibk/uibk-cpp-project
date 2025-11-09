#pragma once
#include <vector>
#include <SFML/Graphics.hpp>

class MapState
{
  public:
	explicit MapState(sf::Vector2f size);

	void addWall(float x, float y, float w, float h);
	void addSpawnPoint(sf::Vector2f spawn);
	[[nodiscard]] bool isColliding(const sf::RectangleShape &r) const;
	[[nodiscard]] const std::vector<sf::RectangleShape> &getWalls() const;
	[[nodiscard]] const std::vector<sf::Vector2f> &getSpawns() const;

  private:
	sf::Vector2f m_size;
	std::vector<sf::RectangleShape> m_walls;
	std::vector<sf::Vector2f> m_spawns;
};
