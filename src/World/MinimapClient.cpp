#include "MinimapClient.h"
#include "GameConfig.h"

namespace
{
constexpr float MINIMAP_SIZE = 150.f;
constexpr float MINIMAP_MARGIN = 15.f;
constexpr float PLAYER_DOT_RADIUS = 4.f;
constexpr float OWN_PLAYER_DOT_RADIUS = PLAYER_DOT_RADIUS + 2.f;
constexpr float BORDER_THICKNESS = 2.f;
constexpr int SAFE_ZONE_POINT_COUNT = 64;

sf::Vector2f computeMinimapSize(sf::Vector2f mapSize, float baseSize)
{
	float const maxDim = std::max(mapSize.x, mapSize.y);
	return {baseSize * (mapSize.y / maxDim), baseSize * (mapSize.x / maxDim)};
}

float computeScale(sf::Vector2f mapSize, float baseSize)
{
	return baseSize / std::max(mapSize.x, mapSize.y);
}
} // namespace

MinimapClient::MinimapClient(sf::Vector2f mapSize, sf::Vector2f screenSize)
	: m_mapSize(mapSize), m_minimapSize(computeMinimapSize(mapSize, MINIMAP_SIZE)),
	  m_position(screenSize.x - m_minimapSize.x - MINIMAP_MARGIN, MINIMAP_MARGIN),
	  m_scale(computeScale(mapSize, MINIMAP_SIZE))
{
	m_background.setSize(m_minimapSize);
	m_background.setFillColor(sf::Color(30, 30, 30, 200));
	m_background.setPosition(m_position);

	m_border.setSize(m_minimapSize);
	m_border.setFillColor(sf::Color::Transparent);
	m_border.setOutlineColor(sf::Color(100, 100, 100));
	m_border.setOutlineThickness(BORDER_THICKNESS);
	m_border.setPosition(m_position);

	for(size_t i = 0; i < MAX_PLAYERS; ++i)
	{
		m_playerDots[i].setRadius(PLAYER_DOT_RADIUS);
		m_playerDots[i].setOrigin({PLAYER_DOT_RADIUS, PLAYER_DOT_RADIUS});
		m_playerDots[i].setFillColor(PLAYER_COLORS[i]);
		m_playerDots[i].setOutlineColor(sf::Color::White);
		m_playerDots[i].setOutlineThickness(1.f);
	}

	m_safeZoneCircle.setPointCount(SAFE_ZONE_POINT_COUNT);
	m_safeZoneCircle.setFillColor(sf::Color::Transparent);
	m_safeZoneCircle.setOutlineColor(sf::Color(255, 50, 50, 200));
	m_safeZoneCircle.setOutlineThickness(2.f);
}

sf::Vector2f MinimapClient::worldToMinimap(sf::Vector2f worldPos) const
{
	sf::Vector2f const rotatedPos(m_mapSize.y - worldPos.y, worldPos.x);
	return m_position + rotatedPos * m_scale;
}

void MinimapClient::updatePlayers(std::array<PlayerState, MAX_PLAYERS> const &players, EntityId ownPlayerId)
{
	if(m_ownPlayerId != ownPlayerId)
	{
		// Reset the previous own-player dot back to normal size
		if(m_ownPlayerId > 0 && m_ownPlayerId <= MAX_PLAYERS)
		{
			auto &prevDot = m_playerDots[m_ownPlayerId - 1];
			prevDot.setRadius(PLAYER_DOT_RADIUS);
			prevDot.setOrigin({PLAYER_DOT_RADIUS, PLAYER_DOT_RADIUS});
			prevDot.setOutlineThickness(1.f);
		}

		m_ownPlayerId = ownPlayerId;

		// Enlarge the new own-player dot
		if(m_ownPlayerId > 0 && m_ownPlayerId <= MAX_PLAYERS)
		{
			auto &ownDot = m_playerDots[m_ownPlayerId - 1];
			ownDot.setRadius(OWN_PLAYER_DOT_RADIUS);
			ownDot.setOrigin({OWN_PLAYER_DOT_RADIUS, OWN_PLAYER_DOT_RADIUS});
			ownDot.setOutlineThickness(2.f);
		}
	}

	for(size_t i = 0; i < MAX_PLAYERS; ++i)
	{
		auto const &player = players[i];
		if(player.m_id == 0 || !player.isAlive())
		{
			m_playerDots[i].setPosition({-100.f, -100.f});
			continue;
		}

		sf::Vector2f const cartCenter = player.getPosition() + PlayerState::logicalDimensions / 2.f;
		m_playerDots[i].setPosition(worldToMinimap(cartCenter));
	}
}

void MinimapClient::setPosition(sf::Vector2f pos)
{
	m_position = pos;
	m_background.setPosition(m_position);
	m_border.setPosition(m_position);
}

void MinimapClient::setSize(sf::Vector2f size)
{
	float const baseSize = std::max(size.x, size.y);
	m_minimapSize = computeMinimapSize(m_mapSize, baseSize);
	m_scale = computeScale(m_mapSize, baseSize);
	m_background.setSize(m_minimapSize);
	m_border.setSize(m_minimapSize);
}

void MinimapClient::updateSafeZone(SafeZone const &zone)
{
	if(!zone.isActive)
	{
		m_safeZoneCircle.setRadius(0.f);
		return;
	}

	sf::Vector2f const minimapCenter = worldToMinimap(zone.center);
	float const scaledRadius = zone.currentRadius * m_scale;

	m_safeZoneCircle.setRadius(scaledRadius);
	m_safeZoneCircle.setOrigin({scaledRadius, scaledRadius});
	m_safeZoneCircle.setPosition(minimapCenter);
}

void MinimapClient::draw(sf::RenderTarget &target, sf::RenderStates states) const
{
	target.draw(m_background, states);
	target.draw(m_border, states);
	target.draw(m_safeZoneCircle, states);

	// Draw other players first, then own player on top
	for(size_t i = 0; i < MAX_PLAYERS; ++i)
	{
		if(i + 1 != m_ownPlayerId)
			target.draw(m_playerDots[i], states);
	}

	if(m_ownPlayerId > 0 && m_ownPlayerId <= MAX_PLAYERS)
		target.draw(m_playerDots[m_ownPlayerId - 1], states);
}
