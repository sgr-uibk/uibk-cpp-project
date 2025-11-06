#pragma once
#include <iostream>
#include <fstream>
#include <SFML/Graphics.hpp>
#include "Map.h"
#include "Player.h"
#include "HealthBar.h"

// This class owns all objects relevant for the local player
// Note: The server uses this class too, so no graphics related code in here!
class GameWorld {
public:
	GameWorld(sf::Vector2u windowDimensions);

	void update(float dt);
	void draw(sf::RenderWindow& window) const;

	bool loadMap(const std::string& filename, float tileSize = 48.f);

	// Expose player for networking...?
	Player& getLocalPlayer() { return m_player; }
	const Map& getMap() const { return m_map; }

	// not const, because it get changed in updateView
	sf::View& getWorldView() { return m_worldView; }

private:
	sf::Vector2u m_windowDimensions;

	Map m_map;
	Player m_player;
	HealthBar m_healthbar;

	sf::View m_worldView;
	sf::View m_hudView;
};
