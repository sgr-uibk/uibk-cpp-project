#pragma once
#include <SFML/Network.hpp>
#include <SFML/Graphics.hpp>

#include "Networking.h"
#include "Map/MapState.h"

struct PlayerState
{
	PlayerState() = default;
	PlayerState(sf::Packet pkt);
	PlayerState(uint32_t id, sf::Vector2f pos, int maxHealth = 100);
	PlayerState(uint32_t id, sf::Vector2f pos, sf::Angle rot, int maxHealth = 100);

	// Simulation
	void update(float dt);
	void moveOn(MapState const &map, sf::Vector2f posDelta);
	void setRotation(sf::Angle);
	void takeDamage(int amount);
	void heal(int amount);
	void die();
	void revive();

	[[nodiscard]] uint32_t getPlayerId() const;
	[[nodiscard]] sf::Vector2f getPosition() const;
	[[nodiscard]] sf::Angle getRotation() const;
	[[nodiscard]] int getHealth() const;
	[[nodiscard]] int getMaxHealth() const;
	[[nodiscard]] bool isAlive() const;

	void serialize(sf::Packet &pkt) const;
	void deserialize(sf::Packet &pkt);

	EntityId m_id;
	sf::Vector2f m_pos;
	sf::Angle m_rot;
	int m_maxHealth;
	int m_health = m_maxHealth;
	static constexpr sf::Vector2f logicalDimensions = {32, 32};
};
