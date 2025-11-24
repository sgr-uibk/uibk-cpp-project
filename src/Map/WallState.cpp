#include "WallState.h"
#include "Networking.h"
#include <algorithm>
#include <spdlog/spdlog.h>

WallState::WallState()
	: m_shape(), m_health(DEFAULT_WALL_HEALTH), m_maxHealth(DEFAULT_WALL_HEALTH), m_isDestroyed(false)
{
	m_shape.setFillColor(sf::Color::Black);
}

WallState::WallState(float x, float y, float w, float h, int maxHealth)
	: m_shape({w, h}), m_health(maxHealth), m_maxHealth(maxHealth), m_isDestroyed(false)
{
	m_shape.setPosition({x, y});
	m_shape.setFillColor(sf::Color::Black);
}

void WallState::takeDamage(int amount)
{
	if(m_isDestroyed)
		return;

	m_health -= amount;
	if(m_health <= 0)
	{
		m_health = 0;
		m_isDestroyed = true;
	}
	updateVisuals();
}

void WallState::repair(int amount)
{
	if(m_isDestroyed)
		return;

	m_health = std::min(m_maxHealth, m_health + amount);
	updateVisuals();
}

void WallState::destroy()
{
	m_health = 0;
	m_isDestroyed = true;
	updateVisuals();
}

bool WallState::isDestroyed() const
{
	return m_isDestroyed;
}

int WallState::getHealth() const
{
	return m_health;
}

int WallState::getMaxHealth() const
{
	return m_maxHealth;
}

float WallState::getHealthPercent() const
{
	if(m_maxHealth == 0)
		return 0.0f;
	return static_cast<float>(m_health) / static_cast<float>(m_maxHealth);
}

sf::FloatRect WallState::getGlobalBounds() const
{
	return m_shape.getGlobalBounds();
}

const sf::RectangleShape &WallState::getShape() const
{
	return m_shape;
}

sf::RectangleShape &WallState::getShape()
{
	return m_shape;
}

void WallState::updateVisuals()
{
	if(m_isDestroyed)
	{
		m_shape.setFillColor(sf::Color(0, 0, 0, 0));
	}
	else
	{
		float healthPercent = getHealthPercent();
		uint8_t alpha = static_cast<uint8_t>(100 + healthPercent * 155);
		uint8_t brightness = static_cast<uint8_t>(healthPercent * 255);
		m_shape.setFillColor(sf::Color(brightness / 2, brightness / 2, brightness / 2, alpha));
	}
}

void WallState::serialize(sf::Packet &pkt) const
{
	pkt << m_shape.getPosition();
	pkt << m_shape.getSize();
	pkt << static_cast<int32_t>(m_health);
	pkt << static_cast<int32_t>(m_maxHealth);
	pkt << m_isDestroyed;
}

void WallState::deserialize(sf::Packet &pkt)
{
	sf::Vector2f position, size;
	int32_t health, maxHealth;

	pkt >> position;
	pkt >> size;
	pkt >> health;
	pkt >> maxHealth;
	pkt >> m_isDestroyed;

	m_shape.setPosition(position);
	m_shape.setSize(size);
	m_health = health;
	m_maxHealth = maxHealth;

	updateVisuals();
}
