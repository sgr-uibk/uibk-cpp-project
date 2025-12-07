#include "ItemClient.h"
#include "Utilities.h"
#include <cmath>

ItemClient::ItemClient(const ItemState &state)
	: m_state(state), m_shape(sf::Vector2f(20.f, 20.f)), m_bobPhase(0.f), m_bobSpeed(2.f), m_bobHeight(5.f)
{
	m_shape.setFillColor(getColorForType(state.getType()));
	m_shape.setOutlineColor(sf::Color::White);
	m_shape.setOutlineThickness(2.f);
	m_shape.setOrigin(sf::Vector2f(10.f, 10.f));
	syncSpriteToState(state);
}

void ItemClient::syncSpriteToState(const ItemState &state)
{
	m_state = state;
	sf::Vector2f isoPos = cartesianToIso(m_state.getPosition());
	m_shape.setPosition(isoPos);
	m_shape.setFillColor(getColorForType(state.getType()));
}

void ItemClient::update(float dt)
{
	// bobbing animation
	m_bobPhase += dt * m_bobSpeed * 2.f * 3.14159f;
	if(m_bobPhase > 2.f * 3.14159f)
	{
		m_bobPhase -= 2.f * 3.14159f;
	}

	float bobOffset = std::sin(m_bobPhase) * m_bobHeight;
	sf::Vector2f basePos = m_state.getPosition();
	sf::Vector2f isoPos = cartesianToIso(basePos);
	m_shape.setPosition(sf::Vector2f(isoPos.x, isoPos.y + bobOffset));
}

sf::Color ItemClient::getColorForType(PowerupType type)
{
	switch(type)
	{
	case PowerupType::HEALTH_PACK:
		return sf::Color(0, 255, 0); // green
	case PowerupType::SPEED_BOOST:
		return sf::Color(0, 150, 255); // blue
	case PowerupType::DAMAGE_BOOST:
		return sf::Color(255, 100, 0); // orange
	case PowerupType::SHIELD:
		return sf::Color(200, 200, 255); // purple
	case PowerupType::RAPID_FIRE:
		return sf::Color(255, 255, 0); // yellow
	default:
		return sf::Color(128, 128, 128); // gray
	}
}