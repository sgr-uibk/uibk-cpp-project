#include "SafeZoneClient.h"
#include "Utilities.h"
#include <cmath>

namespace
{
constexpr float ZONE_OUTLINE_THICKNESS = 4.f;
constexpr float TWO_PI = 6.283185f;
constexpr sf::Color ZONE_OUTLINE_COLOR{255, 50, 50, 200};
} // namespace

SafeZoneClient::SafeZoneClient()
{
	m_zoneCircle.setFillColor(sf::Color::Transparent);
	m_zoneCircle.setOutlineColor(ZONE_OUTLINE_COLOR);
	m_zoneCircle.setOutlineThickness(ZONE_OUTLINE_THICKNESS);
}

void SafeZoneClient::update(SafeZone const &zone, sf::Vector2f const playerPos)
{
	m_isActive = zone.isActive;
	if(!m_isActive)
	{
		m_playerOutside = false;
		return;
	}

	m_playerOutside = zone.isOutside(playerPos);

	m_pulseTimer += 0.05f;
	if(m_pulseTimer > TWO_PI)
		m_pulseTimer -= TWO_PI;

	sf::Vector2f const isoCenter = cartesianToIso(zone.center);

	float const isoRadius = zone.currentRadius * 1.41f;

	m_zoneCircle.setPointCount(64);
	m_zoneCircle.setRadius(isoRadius);
	m_zoneCircle.setOrigin({isoRadius, isoRadius});
	m_zoneCircle.setPosition(isoCenter);
	m_zoneCircle.setScale({1.f, 0.5f});
}

void SafeZoneClient::draw(sf::RenderTarget &target, sf::RenderStates states) const
{
	if(!m_isActive)
		return;

	target.draw(m_zoneCircle, states);
}

void SafeZoneClient::drawDangerOverlay(sf::RenderTarget &target, sf::Vector2f const screenSize) const
{
	if(!m_isActive || !m_playerOutside)
		return;

	float const pulse = 0.5f + 0.5f * std::sin(m_pulseTimer * 3.f);
	auto const alpha = static_cast<std::uint8_t>(60.f + 40.f * pulse);

	sf::RectangleShape overlay(screenSize);
	overlay.setFillColor(sf::Color(200, 0, 0, alpha));
	target.draw(overlay);
}
