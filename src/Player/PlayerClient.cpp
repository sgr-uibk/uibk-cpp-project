#include "PlayerClient.h"
#include "ResourceManager.h"
#include <spdlog/spdlog.h>

PlayerClient::PlayerClient(PlayerState &state, const sf::Color &color)
	: m_state(state), m_color(color), m_healthyTex(TextureManager::inst().load("tank_hul_healthy.png")),
	  m_damagedTex(TextureManager::inst().load("tank_hul_damaged.png")),
	  m_deadTex(TextureManager::inst().load("tank_hul_dead.png")), m_sprite(m_healthyTex),
	  m_cannonHealthyTex(TextureManager::inst().load("cannon_healthy.png")),
	  m_cannonDamagedTex(TextureManager::inst().load("cannon_damaged.png")),
	  m_cannonDeadTex(TextureManager::inst().load("cannon_dead.png")),
	  m_cannonHealthyShootingTex(TextureManager::inst().load("cannon_healthy_shooting.png")),
	  m_cannonDamagedShootingTex(TextureManager::inst().load("cannon_damaged_shooting.png")),
	  m_cannonSprite(m_cannonHealthyTex), m_font(FontManager::inst().load("Font/LiberationSans-Regular.ttf")),
	  m_nameText(m_font, m_state.m_name, 14), m_shootAnimTimer(0.f), m_lastShootCooldown(0.f)
{
	updateSprite();
	m_sprite.setPosition(m_state.m_pos);

	m_cannonSprite.setColor(m_color);
	// set cannon pivot point to exact center of rotation (x=258, y=448 in sprite coordinates)
	m_cannonSprite.setOrigin(sf::Vector2f(258.f, 430.f));
	sf::Vector2f const cannonTexSize = sf::Vector2f(m_cannonSprite.getTexture().getSize()) / 2.f;
	sf::Vector2f const cannonScale = PlayerState::logicalDimensions.componentWiseDiv(cannonTexSize) * 1.3f;
	m_cannonSprite.setScale(cannonScale);

	m_nameText.setFillColor(sf::Color::White);
	m_nameText.setOutlineColor(sf::Color::Black);
	m_nameText.setOutlineThickness(1.f);
	updateNameText();
}

void PlayerClient::update(float dt)
{
	float currentCooldown = m_state.m_shootCooldown.getRemaining();
	if(m_lastShootCooldown == 0.f && currentCooldown > 0.f)
	{
		m_shootAnimTimer = SHOOT_ANIM_DURATION;
	}
	m_lastShootCooldown = currentCooldown;

	if(m_shootAnimTimer > 0.f)
	{
		m_shootAnimTimer -= dt;
		if(m_shootAnimTimer < 0.f)
			m_shootAnimTimer = 0.f;
	}

	// smoothing / interpolation can be inserted here
	syncSpriteToState();
	updateNameText();
}

void PlayerClient::draw(sf::RenderWindow &window) const
{
	window.draw(m_sprite);
	window.draw(m_cannonSprite);
	window.draw(m_nameText);
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

void PlayerClient::applyLocalMove(MapState const &map, sf::Vector2f delta)
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
	sf::Vector2f center = m_state.m_pos + sf::Vector2f(PlayerState::logicalDimensions / 2.f);

	m_sprite.setPosition(center);
	m_sprite.setRotation(m_state.m_rot);

	m_cannonSprite.setPosition(center);
	m_cannonSprite.setRotation(m_state.m_cannonRot);

	bool showShootingBlast = (m_shootAnimTimer > 0.f);

	if(m_state.m_health <= 0)
	{
		m_sprite.setTexture(m_deadTex);
		m_cannonSprite.setTexture(m_cannonDeadTex);
	}
	else if(m_state.m_health < m_state.m_maxHealth / 2)
	{
		m_sprite.setTexture(m_damagedTex);
		m_cannonSprite.setTexture(showShootingBlast ? m_cannonDamagedShootingTex : m_cannonDamagedTex);
	}
	else
	{
		m_sprite.setTexture(m_healthyTex);
		m_cannonSprite.setTexture(showShootingBlast ? m_cannonHealthyShootingTex : m_cannonHealthyTex);
	}
}

void PlayerClient::updateNameText()
{
	sf::FloatRect textBounds = m_nameText.getLocalBounds();
	sf::Vector2f tankCenter = m_state.m_pos + sf::Vector2f(PlayerState::logicalDimensions / 2.f);

	sf::Vector2f textPos(tankCenter.x - textBounds.size.x / 2.f - textBounds.position.x,
	                     tankCenter.y - tankDimensions.y / 2.f - 18.f // 18 pixels above tank
	);
	m_nameText.setPosition(textPos);
}
