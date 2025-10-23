#include "Player.h"
#include <algorithm>
#include <cmath>

#include "Map.h"

Player::Player(const std::string &name,
               const sf::Color &color,
               Map &map,
               float maxHealth)
	: m_name(name)
	  , m_color(color)
	  , m_health(maxHealth)
	  , m_maxHealth(std::max(1.f, maxHealth))
	  , m_map(map)
	  , m_healthyTex("../assets/tank_healthy.png")
	  , m_damagedTex("../assets/tank_damaged.png")
	  , m_destroyedTex("../assets/tank_destroyed.png")
	  , m_sprite(m_healthyTex)
	  , m_onHealthChanged(nullptr)
{
	updateSprite();
	m_sprite.setPosition({368.f + tankW / 2.f, 268.f + tankH / 2.f});
}

void Player::update(float /*dt*/)
{
	// Manage item cooldowns
	// Send keepalive ?
	// Consume fuel ?
}

void Player::draw(sf::RenderWindow &window) const
{
	window.draw(m_sprite);
}

void Player::movement(const sf::Vector2f &delta)
{
	if(m_health <= 0)
		return;
	if(delta.x == 0 && delta.y == 0)
		return;

	constexpr float tankMoveSpeed = 5.f;
	// new center coordinates
	sf::Vector2f const TankPosNew = m_sprite.getPosition() + delta.normalized() * tankMoveSpeed;
	// we need top-left coordinates for the Rect
	sf::FloatRect newTankRect({TankPosNew.x - tankW / 2.f, TankPosNew.y - tankH / 2.f}, {tankW, tankH});

	bool collision = m_map.isCollidingWithWalls(newTankRect);

	if(!collision)
	{
		// no collision, let them move there
		m_sprite.setPosition(TankPosNew);
	}
	else
	{
		// player crashed against something, deal them damage
		takeDamage(1);
	}

	// face towards movement direction (0 deg = up, right hand side coordinate system)
	// atan2 preserves signs as it takes a 2D vector, which atan can't as it only takes the ratio.
	float const angRad = std::atan2(delta.x, -delta.y);
	m_sprite.setRotation(sf::radians(angRad));

}

void Player::takeDamage(float amount)
{
	if(m_health <= 0 || amount <= 0.f)
		return;
	setHealthInternal(m_health - amount);
}

void Player::heal(float amount)
{
	if(amount <= 0.f)
		return;
	setHealthInternal(m_health + amount);
}

void Player::die()
{
	updateSprite(&m_destroyedTex); // damaging
	auto color = m_sprite.getColor();
	color.a *= 0.5f; // fade alpha value to distinguish from living players
	m_sprite.setColor(color);
}

void Player::revive()
{
	setHealthInternal(m_maxHealth);
	m_sprite.setTexture(m_healthyTex);
	m_sprite.setColor(m_color);
	if(m_onHealthChanged)
		m_onHealthChanged(m_health, m_maxHealth);
}

void Player::updateSprite(sf::Texture const *const tex)
{
	if(tex)
		m_sprite.setTexture(*tex);
	m_sprite.setColor(m_color); // texture files are just gray, here we apply the color
	auto const texSize = m_sprite.getTexture().getSize();
	m_sprite.setOrigin({texSize.x / 2.f, texSize.y / 2.f}); // center the origin
	m_sprite.setScale({tankW / texSize.x, tankH / texSize.y});
}

float Player::getHealth() const
{
	return m_health;
}

float Player::getMaxHealth() const
{
	return m_maxHealth;
}

const std::string &Player::getName() const
{
	return m_name;
}

const sf::Sprite &Player::getSprite() const
{
	return m_sprite;
}

void Player::setHealthCallback(HealthCallback cb)
{
	m_onHealthChanged = std::move(cb);
}

void Player::setPosition(const sf::Vector2f &pos)
{
	m_sprite.setPosition(pos);
}

sf::Vector2f Player::getPosition() const
{
	return m_sprite.getPosition();
}

void Player::setHealthInternal(float hp)
{
	float clamped = std::clamp(hp, 0.f, m_maxHealth);
	float const ratio_old = m_health / m_maxHealth;
	float const ratio_new = clamped / m_maxHealth;
	if(clamped == m_health)
		return;

	m_health = clamped;
	if(m_health <= 0.f)
		die();
	else if(ratio_new <= 0.6f && ratio_old > 0.6f)
		updateSprite(&m_damagedTex); // damaging
	else if(ratio_new > 0.6f && ratio_old < 0.6f)
		updateSprite(&m_healthyTex); // healing

	if(m_onHealthChanged)
		m_onHealthChanged(m_health, m_maxHealth);
}
