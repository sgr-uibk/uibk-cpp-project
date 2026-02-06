#include "MinimapClient.h"
#include "GameConfig.h"
#include "Networking.h"

namespace
{
constexpr float MINIMAP_SIZE = 150.f;
constexpr float MINIMAP_MARGIN = 15.f;
constexpr float PLAYER_DOT_RADIUS = 4.f;
constexpr float BORDER_THICKNESS = 2.f;
} // namespace

MinimapClient::MinimapClient(sf::Vector2f mapSize, sf::Vector2f screenSize)
	: m_mapSize(mapSize),
	  m_minimapSize(MINIMAP_SIZE * (mapSize.y / std::max(mapSize.x, mapSize.y)),
	                MINIMAP_SIZE * (mapSize.x / std::max(mapSize.x, mapSize.y))),
	  m_position(screenSize.x - m_minimapSize.x - MINIMAP_MARGIN, MINIMAP_MARGIN),
	  m_scale(MINIMAP_SIZE / std::max(mapSize.x, mapSize.y))
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

	m_safeZoneCircle.setPointCount(64);
	m_safeZoneCircle.setFillColor(sf::Color::Transparent);
	m_safeZoneCircle.setOutlineColor(sf::Color(255, 50, 50, 200));
	m_safeZoneCircle.setOutlineThickness(2.f);
}

void MinimapClient::updatePlayers(std::array<PlayerState, MAX_PLAYERS> const &players, EntityId ownPlayerId)
{
	m_ownPlayerId = ownPlayerId;

	for(size_t i = 0; i < MAX_PLAYERS; ++i)
	{
		auto const &player = players[i];
		if(player.m_id == 0 || !player.isAlive())
		{
			m_playerDots[i].setPosition({-100.f, -100.f});
			continue;
		}

		sf::Vector2f cartPos = player.getPosition() + PlayerState::logicalDimensions / 2.f;
		sf::Vector2f rotatedPos(m_mapSize.y - cartPos.y, cartPos.x);
		sf::Vector2f minimapPos = m_position + rotatedPos * m_scale;

		m_playerDots[i].setPosition(minimapPos);

		if(player.m_id == ownPlayerId)
		{
			m_playerDots[i].setRadius(PLAYER_DOT_RADIUS + 2.f);
			m_playerDots[i].setOrigin({PLAYER_DOT_RADIUS + 2.f, PLAYER_DOT_RADIUS + 2.f});
			m_playerDots[i].setOutlineThickness(2.f);
		}
		else
		{
			m_playerDots[i].setRadius(PLAYER_DOT_RADIUS);
			m_playerDots[i].setOrigin({PLAYER_DOT_RADIUS, PLAYER_DOT_RADIUS});
			m_playerDots[i].setOutlineThickness(1.f);
		}
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
	float const maxDim = std::max(size.x, size.y);
	m_minimapSize = {maxDim * (m_mapSize.y / std::max(m_mapSize.x, m_mapSize.y)),
	                 maxDim * (m_mapSize.x / std::max(m_mapSize.x, m_mapSize.y))};
	m_scale = maxDim / std::max(m_mapSize.x, m_mapSize.y);
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

	sf::Vector2f const rotatedCenter(m_mapSize.y - zone.center.y, zone.center.x);
	sf::Vector2f const minimapCenter = m_position + rotatedCenter * m_scale;
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

	for(size_t i = 0; i < MAX_PLAYERS; ++i)
	{
		if(i + 1 != m_ownPlayerId)
			target.draw(m_playerDots[i], states);
	}

	if(m_ownPlayerId > 0 && m_ownPlayerId <= MAX_PLAYERS)
		target.draw(m_playerDots[m_ownPlayerId - 1], states);
}
