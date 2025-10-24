#include "Player.h"
#include <algorithm>
#include <cmath>
#include <utility>

#include "Map.h"

Player::Player(std::string name,
               sf::Color const &color,
               Map &map,
               int const maxHealth)
	: m_name(std::move(name)),
	  m_color(color),
	  m_maxHealth(maxHealth),
	  m_health(maxHealth), m_midThreshold(3 * maxHealth / 4),
	  m_lowThreshold(maxHealth / 4),
	  m_map(map),
	  m_healthyTex("../assets/tank_healthy.png"),
	  m_damagedTex("../assets/tank_damaged.png"),
	  m_deadTex("../assets/tank_dead.png"),
	  m_sprite(m_healthyTex),
	  m_onHealthChanged(nullptr)
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

void Player::takeDamage(int amount)
{
	if(m_health <= 0 || amount <= 0)
		return;
	setHealthInternal(m_health - amount);
}

void Player::heal(int amount)
{
	if(amount <= 0)
		return;
	setHealthInternal(m_health + amount);
}

void Player::die()
{
	updateSprite(&m_deadTex); // damaging
	auto color = m_sprite.getColor();
	color.a /= 2; // fade alpha value to distinguish from living players
	m_sprite.setColor(color);
}

void Player::revive()
{
	if(m_health > 0)
		return;

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

int Player::getHealth() const
{
	return m_health;
}

int Player::getMaxHealth() const
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

void Player::setHealthInternal(int health)
{
	int const clamped = std::clamp(health, 0, m_maxHealth);
	if(clamped == m_health)
		return;

	int const thresh_mid = m_maxHealth / 2;
	int const thresh_high = 3 * m_maxHealth / 4;

	if(health <= thresh_mid && m_health > thresh_mid)
		updateSprite(&m_damagedTex); // damaging
	else if(health >= thresh_high && m_health < thresh_high)
		updateSprite(&m_healthyTex); // healing

	m_health = clamped;
	if(m_health == 0)
		die();


	if(m_onHealthChanged)
		m_onHealthChanged(m_health, m_maxHealth);
}
