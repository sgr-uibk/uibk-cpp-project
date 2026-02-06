#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <cstdint>
#include "../GameConfig.h"

class ProjectileState
{
  public:
	ProjectileState();
	ProjectileState(uint32_t id, sf::Vector2f position, sf::Vector2f velocity, uint32_t ownerId,
	                int damage = GameConfig::Projectile::BASE_DAMAGE);

	void update(float dt);

	uint32_t getId() const
	{
		return m_id;
	}
	sf::Vector2f getPosition() const
	{
		return m_position;
	}
	sf::Vector2f getVelocity() const
	{
		return m_velocity;
	}
	uint32_t getOwnerId() const
	{
		return m_ownerId;
	}
	int getDamage() const
	{
		return m_damage;
	}
	bool isActive() const
	{
		return m_active;
	}
	float getLifetime() const
	{
		return m_lifetime;
	}

	void setActive(bool active)
	{
		m_active = active;
	}
	void deactivate()
	{
		m_active = false;
	}

	sf::FloatRect getBounds() const;

	void serialize(sf::Packet &packet) const;
	void deserialize(sf::Packet &packet);

  private:
	uint32_t m_id;
	sf::Vector2f m_position;
	sf::Vector2f m_velocity;
	uint32_t m_ownerId;
	int m_damage;
	bool m_active;
	float m_lifetime;

	static constexpr float COLLISION_SIZE = 8.f;
	static constexpr float MAX_LIFETIME = 5.f;
};
