#pragma once
#include <SFML/Network.hpp>
#include <SFML/Graphics.hpp>
#include <array>

#include "Networking.h"
#include "Map/MapState.h"
#include "Cooldown.h"
#include "Powerup.h"
#include "GameConfig.h"

enum class PlayerMode
{
    NORMAL,
    SPECTATOR
};

struct PlayerState
{
	PlayerState() = default;
	PlayerState(sf::Packet pkt);
	PlayerState(uint32_t id, sf::Vector2f pos, int maxHealth = 100);
	PlayerState(uint32_t id, sf::Vector2f pos, sf::Angle rot, int maxHealth = 100);
	PlayerMode m_mode = PlayerMode::NORMAL;

	// Simulation
	void update(float dt);
	void moveOn(MapState const &map, sf::Vector2f posDelta);
	void setRotation(sf::Angle);
	void setPosition(const sf::Vector2f& pos);
	void takeDamage(int amount);
	void heal(int amount);
	void die();
	void revive();

	bool canShoot() const;
	void shoot();

	void applyPowerup(PowerupType type);
	bool hasPowerup(PowerupType type) const;
	const PowerupEffect *getPowerup(PowerupType type) const;
	float getSpeedMultiplier() const;
	int getDamageMultiplier() const;
	float getShootCooldown() const;

	bool addToInventory(PowerupType type);
	void useSelectedItem();
	void setSelectedSlot(int slot);
	int getSelectedSlot() const
	{
		return m_selectedSlot;
	}
	PowerupType getInventoryItem(int slot) const;
	const std::array<PowerupType, 9> &getInventory() const
	{
		return m_inventory;
	}

	[[nodiscard]] uint32_t getPlayerId() const;
	[[nodiscard]] sf::Vector2f getPosition() const;
	[[nodiscard]] sf::Angle getRotation() const;
	[[nodiscard]] int getHealth() const;
	[[nodiscard]] int getMaxHealth() const;
	[[nodiscard]] bool isAlive() const;

	void serialize(sf::Packet &pkt) const;
	void deserialize(sf::Packet &pkt);

	EntityId m_id;
	std::string m_name;
	sf::Vector2f m_pos;
	sf::Angle m_rot;
	int m_maxHealth;
	int m_health = m_maxHealth;
	Cooldown m_shootCooldown{GameConfig::Player::SHOOT_COOLDOWN};

	static constexpr int MAX_POWERUPS = 5;
	std::array<PowerupEffect, MAX_POWERUPS> m_powerups;

	std::array<PowerupType, 9> m_inventory{PowerupType::NONE};
	int m_selectedSlot = 0;

	static constexpr sf::Vector2f logicalDimensions = {32, 32};
};