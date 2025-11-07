#include "PlayerClient.h"
#include "ResourceManager.h"

PlayerClient::PlayerClient(const PlayerState &state, const sf::Color &color)
	: m_state(state),
	  m_color(color),
	  m_healthyTex(TextureManager::inst().load("tank_healthy.png")),
	  m_damagedTex(TextureManager::inst().load("tank_damaged.png")),
	  m_deadTex(TextureManager::inst().load("tank_dead.png")),
	  m_sprite(m_healthyTex)
{
	updateSprite();
	m_sprite.setPosition(m_state.m_pos);
}

void PlayerClient::update(float)
{
	// smoothing / interpolation can be inserted here
	syncSpriteToState();
}

void PlayerClient::draw(sf::RenderWindow &window) const
{
	window.draw(m_sprite);
}

void PlayerClient::applyServerState(const PlayerState &serverState)
{
	int prevHealth = m_state.m_health;
	m_state = serverState;
	if(m_onHealthChanged && prevHealth != m_state.m_health)
	{
		m_onHealthChanged(m_state.m_health, m_state.m_maxHealth);
	}
	syncSpriteToState();
}

void PlayerClient::applyLocalMove(MapState const& map, sf::Vector2f delta)
{
	m_state.moveOn(map, delta);
	// prediction using same logic as server (map pointer should point to local map copy)
	syncSpriteToState();
}

void PlayerClient::registerHealthCallback(HealthCallback cb)
{
	m_onHealthChanged = std::move(cb);
}

void PlayerClient::updateSprite()
{
	syncSpriteToState();
	m_sprite.setColor(m_color); // texture files are just gray, here we apply the color
	auto const texSize = sf::Vector2f(m_sprite.getTexture().getSize()) / 2.f;
	m_sprite.setOrigin(texSize); // center the origin
	sf::Vector2f const scale = PlayerState::logicalDimensions.componentWiseDiv(texSize);
	m_sprite.setScale(scale);
}

void PlayerClient::syncSpriteToState()
{
	m_sprite.setPosition(m_state.m_pos + sf::Vector2f(PlayerState::logicalDimensions / 2.f));
	m_sprite.setRotation(m_state.m_rot);

	if(m_state.m_health <= 0)
		m_sprite.setTexture(m_deadTex);
	else if(m_state.m_health < m_state.m_maxHealth / 2)
		m_sprite.setTexture(m_damagedTex);
	else
		m_sprite.setTexture(m_healthyTex);
}
