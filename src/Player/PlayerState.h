#pragma once
#include <SFML/Network.hpp>
#include <SFML/Graphics.hpp>
#include <array>

#include "Networking.h"
#include "Cooldown.h"
#include "Powerup.h"
#include "GameConfig.h"

struct MapState;

struct InterpPlayerState
{
	InterpPlayerState() = default;
	InterpPlayerState(sf::Vector2f const pos, sf::Angle const rot, sf::Angle const cannonRot)
		: m_pos(pos), m_rot(rot), m_cannonRot(cannonRot)
	{
	}
	explicit InterpPlayerState(sf::Packet &pkt)
	{
		pkt >> m_pos >> m_rot >> m_cannonRot;
	}
	void overwriteBy(InterpPlayerState auth);
	sf::Vector2f m_pos;
	sf::Angle m_rot;
	sf::Angle m_cannonRot;
};
sf::Packet &operator>>(sf::Packet &pkt, InterpPlayerState &obj);
sf::Packet &operator<<(sf::Packet &pkt, InterpPlayerState const &obj);

struct PlayerState
{
	PlayerState() = default;
	PlayerState(EntityId id, sf::Vector2f pos, int maxHealth = 100);
	PlayerState(EntityId id, sf::Packet &pkt);
	PlayerState(EntityId id, sf::Vector2f pos, sf::Angle rot, int maxHealth = 100);
	PlayerState(EntityId id, InterpPlayerState ips, int maxHealth = 100);
	void assignSnappedState(PlayerState const &other);
	// Simulation
	void update(float dt);
	sf::Vector2f moveOn(MapState const &map, sf::Vector2f posDelta);
	void setRotation(sf::Angle);
	void takeDamage(int amount);
	void heal(int amount);
	void die();
	void revive();

	bool tryShoot();

	void applyPowerup(PowerupType type);
	bool hasPowerup(PowerupType type) const;
	PowerupEffect const *getPowerup(PowerupType type) const;
	float getSpeedMultiplier() const;
	int getDamageMultiplier() const;
	float getShootCooldown() const;

	bool addToInventory(PowerupType type);
	void useItem(size_t slotNum);
	PowerupType getInventoryItem(int slot) const;
	std::array<PowerupType, 9> const &getInventory() const
	{
		return m_inventory;
	}

	[[nodiscard]] uint32_t getPlayerId() const;
	[[nodiscard]] sf::Vector2f getPosition() const;
	[[nodiscard]] sf::Angle getRotation() const;
	[[nodiscard]] sf::Angle getCannonRotation() const;
	void setCannonRotation(sf::Angle angle);
	[[nodiscard]] int getHealth() const;
	[[nodiscard]] int getMaxHealth() const;
	[[nodiscard]] bool isAlive() const;

	void serialize(sf::Packet &pkt) const;
	void deserialize(sf::Packet &pkt);

	EntityId m_id;
	std::string m_name;
	InterpPlayerState m_iState;
	int m_maxHealth;
	int m_health = m_maxHealth;
	Cooldown m_shootCooldown{};

	static constexpr int MAX_POWERUPS = 5;
	std::array<PowerupEffect, MAX_POWERUPS> m_powerups;
	std::array<PowerupType, 9> m_inventory{PowerupType::NONE};

	static constexpr sf::Vector2f logicalDimensions = {28, 28};
};
