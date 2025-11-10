#include "PlayerClient.h"
#include "ResourceManager.h"

PlayerClient::PlayerClient(PlayerState &state, const sf::Color &color)
	: m_state(state), m_color(color), m_healthyTex(TextureManager::inst().load("tank_healthy.png")),
	  m_damagedTex(TextureManager::inst().load("tank_damaged.png")),
	  m_deadTex(TextureManager::inst().load("tank_dead.png")), m_sprite(m_healthyTex),
	  m_font(FontManager::inst().load("Font/LiberationSans-Regular.ttf")), m_nameText(m_font, m_state.m_name, 14),
	  m_healthBar(sf::Vector2f(0, 0), sf::Vector2f(40.f, 6.f), m_state.m_maxHealth)
{
	updateSprite();
	m_sprite.setPosition(m_state.m_pos);

	m_nameText.setFillColor(sf::Color::White);
	m_nameText.setOutlineColor(sf::Color::Black);
	m_nameText.setOutlineThickness(1.f);
	updateNameText();

	registerHealthCallback([this](int current, int max) { m_healthBar.healthCallback(current, max); });
}

void PlayerClient::update(float dt)
{
	// smoothing / interpolation can be inserted here
	syncSpriteToState();
	updateNameText();
	updateHealthbar(dt);
}

void PlayerClient::draw(sf::RenderWindow &window) const
{
	window.draw(m_sprite);
	window.draw(m_nameText);
	window.draw(m_healthBar);
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
	m_sprite.setPosition(m_state.m_pos + sf::Vector2f(PlayerState::logicalDimensions / 2.f));
	m_sprite.setRotation(m_state.m_rot);

	if(m_state.m_health <= 0)
		m_sprite.setTexture(m_deadTex);
	else if(m_state.m_health < m_state.m_maxHealth / 2)
		m_sprite.setTexture(m_damagedTex);
	else
		m_sprite.setTexture(m_healthyTex);
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

void PlayerClient::updateHealthbar(float dt)
{
	sf::Vector2f tankCenter = m_state.m_pos + PlayerState::logicalDimensions / 2.f;
	sf::Vector2f barPos(tankCenter.x - 20.f, tankCenter.y - PlayerState::logicalDimensions.y / 2.f - 10.f);
	m_healthBar.setPositionScreen(barPos);
	m_healthBar.update(dt);
}
