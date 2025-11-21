#include "Player/PlayerState.h"
#include "Map/MapState.h"
#include "Networking.h"
#include <algorithm>
#include <spdlog/logger.h>

PlayerState::PlayerState(uint32_t id, sf::Vector2f pos, sf::Angle rot, int maxHealth)
	: m_id(id), m_pos(pos), m_rot(rot), m_maxHealth(maxHealth), m_health(maxHealth)
{
}

PlayerState::PlayerState(uint32_t id, sf::Vector2f pos, int maxHealth) : PlayerState(id, pos, sf::degrees(0), maxHealth)
{
}

PlayerState::PlayerState(sf::Packet pkt)
{
	deserialize(pkt);
}

void PlayerState::update()
{
	// cooldowns
}

void PlayerState::moveOn(MapState const &map, sf::Vector2f posDelta)
{
	if(m_health <= 0)
		return;

	constexpr float tankMoveSpeed = 5.f;
	posDelta *= tankMoveSpeed;
	sf::RectangleShape nextShape(logicalDimensions);
	nextShape.setPosition(m_pos + posDelta);

	if(map.isColliding(nextShape))
	{
		// player crashed against something, deal them damage
		takeDamage(10);
	}
	else
	{
		// no collision, let them move there
		m_pos += posDelta;

		float const angRad = std::atan2(posDelta.x, -posDelta.y);
		sf::Angle const rot = sf::radians(angRad);
		m_rot = rot;
	}
}

// void PlayerState::setPosition(sf::Vector2f pos)
// {
// 	m_position = pos;
// }

void PlayerState::setRotation(sf::Angle rot)
{
	m_rot = rot;
}

void PlayerState::takeDamage(int const amount)
{
	m_health = std::max(0, m_health - amount);
}

void PlayerState::heal(int const amount)
{
	m_health = std::max(m_maxHealth, m_health + amount);
}

void PlayerState::die()
{
	m_health = 0;
}

void PlayerState::revive()
{
	m_health = m_maxHealth;
}

uint32_t PlayerState::getPlayerId() const
{
	return m_id;
}

sf::Vector2f PlayerState::getPosition() const
{
	return m_pos;
}

sf::Angle PlayerState::getRotation() const
{
	return m_rot;
}

int PlayerState::getHealth() const
{
	return m_health;
}

int PlayerState::getMaxHealth() const
{
	return m_maxHealth;
}

bool PlayerState::isAlive() const
{
	return m_health > 0;
}

void PlayerState::serialize(sf::Packet &pkt) const
{
	pkt << m_id;
	pkt << m_pos;
	pkt << m_rot;
	pkt << m_health;
	pkt << m_maxHealth;
}

void PlayerState::deserialize(sf::Packet &pkt)
{
	pkt >> m_id;
	pkt >> m_pos;
	pkt >> m_rot;
	pkt >> m_health;
	pkt >> m_maxHealth;
	// assert(m_health >= 0);
	// assert(m_maxHealth >= 0);
}