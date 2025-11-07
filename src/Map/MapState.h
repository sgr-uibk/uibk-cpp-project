#pragma once
#include <vector>
#include <SFML/Graphics.hpp>
#include <map>
#include "Tile.h"

class MapState
{
public:
	explicit MapState(sf::Vector2f size);

	void reset(sf::Vector2f size);
	void addWall(float x, float y, float w, float h);
	[[nodiscard]] bool isColliding(const sf::RectangleShape &r) const;
	[[nodiscard]] const std::vector<sf::RectangleShape> &getWalls() const;

	void setPlayerSpawn(int playerId, sf::Vector2f pos);
    sf::Vector2f getPlayerSpawn(int playerId) const;

	void addTile(TileType type, sf::Vector2f pos, float size);
	std::vector<Tile>& getTiles();

	sf::Vector2f getSize() const;

private:
	sf::Vector2f m_size;
	std::vector<sf::RectangleShape> m_walls;
	std::map<int, sf::Vector2f> m_playerSpawns;
	std::vector<Tile> m_tiles;
};
