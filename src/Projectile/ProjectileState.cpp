#include "ProjectileState.h"
#include "../Networking.h"

ProjectileState::ProjectileState()
	: m_id(0), m_position(0.f, 0.f), m_velocity(0.f, 0.f), m_ownerId(0), m_damage(25), m_active(false), m_lifetime(0.f),
	  m_maxLifetime(MAX_LIFETIME)
{
}

ProjectileState::ProjectileState(uint32_t id, sf::Vector2f position, sf::Vector2f velocity, uint32_t ownerId,
                                 int damage)
	: m_id(id), m_position(position), m_velocity(velocity), m_ownerId(ownerId), m_damage(damage), m_active(true),
	  m_lifetime(0.f), m_maxLifetime(MAX_LIFETIME)
{
}

void ProjectileState::update(float dt)
{
	if(!m_active)
		return;

	m_position += m_velocity * dt;

	m_lifetime += dt;

	if(m_lifetime >= m_maxLifetime)
	{
		m_active = false;
	}
}

sf::FloatRect ProjectileState::getBounds() const
{
	return sf::FloatRect(sf::Vector2f(m_position.x - PROJECTILE_SIZE / 2.f, m_position.y - PROJECTILE_SIZE / 2.f),
	                     sf::Vector2f(PROJECTILE_SIZE, PROJECTILE_SIZE));
}

void ProjectileState::serialize(sf::Packet &packet) const
{
	packet << m_id << m_position << m_velocity << m_ownerId;
	packet << static_cast<int32_t>(m_damage) << m_active << m_lifetime;
}

void ProjectileState::deserialize(sf::Packet &packet)
{
	int32_t damage;
	packet >> m_id;
	packet >> m_position;
	packet >> m_velocity;
	packet >> m_ownerId;
	packet >> damage;
	packet >> m_active;
	packet >> m_lifetime;
	m_damage = damage;
}
