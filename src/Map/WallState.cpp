#include "WallState.h"
#include "Networking.h"
#include <algorithm>
#include <spdlog/spdlog.h>

WallState::WallState() : m_health(DEFAULT_WALL_HEALTH), m_maxHealth(DEFAULT_WALL_HEALTH)
{
	m_shape.setFillColor(sf::Color::Black);
}

WallState::WallState(sf::Vector2f pos, sf::Vector2f dim, int maxHealth)
	: m_shape(dim), m_health(maxHealth), m_maxHealth(maxHealth)
{
	m_shape.setPosition(pos);
	m_shape.setFillColor(sf::Color::Black);
}

void WallState::takeDamage(int amount)
{
	if(m_health == 0)
		return;

	m_health = std::max(0, m_health - amount);
	updateVisuals();
}

void WallState::repair(int amount)
{
	if(m_health == 0)
		return;

	m_health = std::min(m_maxHealth, m_health + amount);
	updateVisuals();
}

void WallState::destroy()
{
	m_health = 0;
	updateVisuals();
}

void WallState::setHealth(int health)
{
	if(m_health == 0)
		return;

	m_health = std::clamp(health, 0, m_maxHealth);
	updateVisuals();
}

bool WallState::isDestroyed() const
{
	return m_health == 0;
}

int WallState::getHealth() const
{
	return m_health;
}

int WallState::getMaxHealth() const
{
	return m_maxHealth;
}

float WallState::getHealthRelative() const
{
	if(m_maxHealth == 0)
		return 0.0f;
	return static_cast<float>(m_health) / static_cast<float>(m_maxHealth);
}

sf::FloatRect WallState::getGlobalBounds() const
{
	return m_shape.getGlobalBounds();
}

sf::RectangleShape const &WallState::getShape() const
{
	return m_shape;
}

sf::RectangleShape &WallState::getShape()
{
	return m_shape;
}

void WallState::updateVisuals()
{
	if(m_health == 0)
	{
		m_shape.setFillColor(sf::Color::Transparent);
		return;
	}

	float const relHealth = getHealthRelative();
	auto const alpha = static_cast<uint8_t>(DAMAGED_ALPHA_MIN + relHealth * DAMAGED_ALPHA_RANGE);
	auto const brightness = static_cast<uint8_t>(relHealth * 255 * BRIGHTNESS_SCALE);
	m_shape.setFillColor(sf::Color(brightness, brightness, brightness, alpha));
}

void WallState::serialize(sf::Packet &pkt) const
{
	pkt << m_shape.getPosition();
	pkt << m_shape.getSize();
	pkt << static_cast<int32_t>(m_health);
	pkt << static_cast<int32_t>(m_maxHealth);
}

void WallState::deserialize(sf::Packet &pkt)
{
	sf::Vector2f position, size;
	int32_t health, maxHealth;

	pkt >> position;
	pkt >> size;
	pkt >> health;
	pkt >> maxHealth;

	m_shape.setPosition(position);
	m_shape.setSize(size);
	m_health = health;
	m_maxHealth = maxHealth;

	updateVisuals();
}
