#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Audio/Music.hpp>

#include "Map.h"
#include "Player.h"
#include "HealthBar.h"

// This class owns all objects relevant for the local player
// Note: The server uses this class too, so no graphics related code in here!
class GameWorld {
public:
	explicit GameWorld(sf::Vector2u windowDimensions);

	void update(float dt);
	void draw(sf::RenderWindow& window) const;

	// Expose player for networking...?
	Player& getLocalPlayer() { return m_player; }
	const Map& getMap() const { return m_map; }

private:
	sf::Vector2u m_windowDimensions;

	Map m_map;
	Player m_player;
	HealthBar m_healthbar;

	sf::View m_worldView;
	sf::View m_hudView;

	sf::Music m_battleMusic;
};
