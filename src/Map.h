#pragma once
#include <iostream>
#include <fstream>
#include <SFML/Graphics.hpp>
#include <vector>
#include <map>

// this is very rudimentary, I need it to get the Player class running.
class Map {
	std::vector<sf::RectangleShape> m_walls;
	sf::Vector2f m_playerSpawn{0.f, 0.f};
	std::map<char, sf::Texture> m_tileTextures;

public:
	Map() = default;

	std::vector<sf::Sprite> m_tiles;

	bool loadTextures(const std::string& tileFolder);
    bool loadFromFile(const std::string& path, float tileSize = 48.f);

	sf::Vector2f getPlayerSpawn() const { return m_playerSpawn; }

	bool isCollidingWithWalls(sf::FloatRect const& it) const;
	const std::vector<sf::RectangleShape>& getWalls() const;
	void draw(sf::RenderWindow& window) const;
};
