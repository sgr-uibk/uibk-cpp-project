#include "ItemClient.h"
#include "PowerupSpriteManager.h"
#include "Utilities.h"
#include <cmath>
#include <numbers>

ItemClient::ItemClient(ItemState const &state)
	: m_sprite(PowerupSpriteManager::inst().getSpriteForType(state.getType()))
{
	syncSpriteToState(state);
}

void ItemClient::syncSpriteToState(ItemState const &state)
{
	m_state = state;
	m_sprite = PowerupSpriteManager::inst().getSpriteForType(state.getType());
	m_sprite.setPosition(cartesianToIso(m_state.getPosition()));
}

void ItemClient::update(float dt)
{
	m_bobPhase += dt * BOB_SPEED * 2.f * std::numbers::pi_v<float>;
	m_bobPhase = sf::radians(m_bobPhase).wrapUnsigned().asRadians();

	float bobOffset = std::sin(m_bobPhase) * BOB_HEIGHT;
	sf::Vector2f isoPos = cartesianToIso(m_state.getPosition());
	m_sprite.setPosition(sf::Vector2f(isoPos.x, isoPos.y + bobOffset));
}
