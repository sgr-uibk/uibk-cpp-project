#pragma once
#include <SFML/Graphics.hpp>
#include "SafeZone.h"

class SafeZoneClient : public sf::Drawable
{
  public:
	SafeZoneClient();

	void update(SafeZone const &zone, sf::Vector2f playerPos);
	void draw(sf::RenderTarget &target, sf::RenderStates states) const override;
	void drawDangerOverlay(sf::RenderTarget &target, sf::Vector2f screenSize) const;

  private:
	sf::CircleShape m_zoneCircle;
	bool m_isActive = false;
	bool m_playerOutside = false;
	float m_pulseTimer = 0.f;
};
