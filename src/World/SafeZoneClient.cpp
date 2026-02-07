#include "SafeZoneClient.h"
#include "Utilities.h"
#include <cmath>
#include <numbers>

namespace
{
constexpr float ZONE_OUTLINE_THICKNESS = 4.f;
constexpr sf::Color ZONE_OUTLINE_COLOR{255, 50, 50, 200};
constexpr int ZONE_POINT_COUNT = 64;

// Isometric projection scales a circle by sqrt(2) horizontally and squashes vertically by 0.5
constexpr float ISO_RADIUS_SCALE = 1.41421356f; // sqrt(2)
constexpr float ISO_Y_SCALE = 0.5f;

// Danger overlay pulse parameters
constexpr float PULSE_SPEED = 3.f;
constexpr float PULSE_BASE_ALPHA = 60.f;
constexpr float PULSE_AMPLITUDE = 40.f;
constexpr float PULSE_TICK = 0.05f;
} // namespace

SafeZoneClient::SafeZoneClient()
{
	m_zoneCircle.setPointCount(ZONE_POINT_COUNT);
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

	constexpr float TWO_PI = 2.f * std::numbers::pi_v<float>;
	m_pulseTimer += PULSE_TICK;
	if(m_pulseTimer > TWO_PI)
		m_pulseTimer -= TWO_PI;

	sf::Vector2f const isoCenter = cartesianToIso(zone.center);
	float const isoRadius = zone.currentRadius * ISO_RADIUS_SCALE;

	m_zoneCircle.setRadius(isoRadius);
	m_zoneCircle.setOrigin({isoRadius, isoRadius});
	m_zoneCircle.setPosition(isoCenter);
	m_zoneCircle.setScale({1.f, ISO_Y_SCALE});
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

	float const pulse = 0.5f + 0.5f * std::sin(m_pulseTimer * PULSE_SPEED);
	auto const alpha = static_cast<std::uint8_t>(PULSE_BASE_ALPHA + PULSE_AMPLITUDE * pulse);

	sf::RectangleShape overlay(screenSize);
	overlay.setFillColor(sf::Color(200, 0, 0, alpha));
	target.draw(overlay);
}
