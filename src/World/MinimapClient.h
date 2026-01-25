#pragma once
#include <SFML/Graphics.hpp>
#include <array>
#include "Player/PlayerState.h"
#include "Map/MapState.h"
#include "Networking.h"
#include "SafeZone.h"

class MinimapClient : public sf::Drawable
{
  public:
	MinimapClient(sf::Vector2f mapSize, sf::Vector2f screenSize);

	void updatePlayers(std::array<PlayerState, MAX_PLAYERS> const &players, EntityId ownPlayerId);
	void updateSafeZone(SafeZone const &zone);
	void setPosition(sf::Vector2f pos);
	void setSize(sf::Vector2f size);

  private:
	void draw(sf::RenderTarget &target, sf::RenderStates states) const override;

	sf::Vector2f m_mapSize;
	sf::Vector2f m_minimapSize;
	sf::Vector2f m_position;
	float m_scale;

	sf::RectangleShape m_background;
	sf::RectangleShape m_border;
	std::array<sf::CircleShape, MAX_PLAYERS> m_playerDots;
	sf::CircleShape m_safeZoneCircle;
	EntityId m_ownPlayerId = 0;
};
