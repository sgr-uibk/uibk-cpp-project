#include "PlayerClient.h"
#include "ResourceManager.h"

PlayerClient::PlayerClient(PlayerState &state, const sf::Color &color)
	: m_state(state), m_color(color), m_healthyTex(TextureManager::inst().load("tank_healthy.png")),
	  m_damagedTex(TextureManager::inst().load("tank_damaged.png")),
	  m_deadTex(TextureManager::inst().load("tank_dead.png")), m_sprite(m_healthyTex),
	  m_font(FontManager::inst().load("Font/LiberationSans-Regular.ttf")), m_nameText(m_font, m_state.m_name, 14)
{
	updateSprite();
	m_sprite.setPosition(m_state.m_pos);

	m_nameText.setFillColor(sf::Color::White);
	m_nameText.setOutlineColor(sf::Color::Black);
	m_nameText.setOutlineThickness(1.f);
	updateNameText();
}

void PlayerClient::update([[maybe_unused]] float dt)
{
	// smoothing / interpolation can be inserted here
	syncSpriteToState();
	updateNameText();
}

void PlayerClient::draw(sf::RenderWindow &window) const
{
	window.draw(m_sprite);
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
	syncSpriteToState();
}

void PlayerClient::registerHealthCallback(HealthCallback cb)
{
	m_onHealthChanged = std::move(cb);
}

sf::Vector2f lerp(sf::Vector2f const &a, sf::Vector2f const &b, float t)
{
	return a + t * (b - a);
}

sf::Angle lerp(sf::Angle const &a, sf::Angle const &b, float t)
{
	auto const delta = (b - a).wrapSigned();
	return a + t * delta;
}

void PlayerClient::interp(PlayerState const &s0, PlayerState const &s1, float const alpha)
{
	this->m_state.m_pos = lerp(s0.m_pos, s1.m_pos, alpha);
	this->m_state.m_rot = lerp(s0.m_rot, s1.m_rot, alpha);
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
