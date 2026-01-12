#include "ItemClient.h"
#include "PowerupSpriteManager.h"
#include "Utilities.h"
#include <cmath>
#include <numbers>

ItemClient::ItemClient(const ItemState &state)
	: m_state(state), m_sprite(PowerupSpriteManager::inst().getSpriteForType(state.getType())), m_bobPhase(0.f),
	  m_bobSpeed(0.05f), m_bobHeight(5.f)
{
	syncSpriteToState(state);
}

void ItemClient::syncSpriteToState(const ItemState &state)
{
	m_state = state;

	m_sprite = PowerupSpriteManager::inst().getSpriteForType(state.getType());

	sf::Vector2f isoPos = cartesianToIso(m_state.getPosition());
	m_sprite.setPosition(isoPos);
}

void ItemClient::update(float dt)
{
	// bobbing animation
	m_bobPhase += dt * m_bobSpeed * 2.f * std::numbers::pi_v<float>;
	m_bobPhase = sf::radians(m_bobPhase).wrapUnsigned().asRadians();

	float bobOffset = std::sin(m_bobPhase) * m_bobHeight;
	sf::Vector2f basePos = m_state.getPosition();
	sf::Vector2f isoPos = cartesianToIso(basePos);
	m_sprite.setPosition(sf::Vector2f(isoPos.x, isoPos.y + bobOffset));
}
