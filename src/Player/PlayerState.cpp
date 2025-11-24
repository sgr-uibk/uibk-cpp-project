#include "Player/PlayerState.h"
#include "Map/MapState.h"
#include "Networking.h"
#include <algorithm>
#include <spdlog/spdlog.h>

PlayerState::PlayerState(uint32_t id, sf::Vector2f pos, sf::Angle rot, int maxHealth, const std::string &name)
	: m_id(id), m_name(name), m_pos(pos), m_rot(rot), m_maxHealth(maxHealth), m_health(maxHealth)
{
}

PlayerState::PlayerState(uint32_t id, sf::Vector2f pos, int maxHealth) : PlayerState(id, pos, sf::degrees(0), maxHealth)
{
}

PlayerState::PlayerState(sf::Packet pkt)
{
	deserialize(pkt);
}

void PlayerState::update(float dt)
{
	// cooldowns
	m_shootCooldown.update(dt);

	for(auto &powerup : m_powerups)
	{
		powerup.update(dt);
	}
}

void PlayerState::moveOn(MapState const &map, sf::Vector2f posDelta)
{
	constexpr float tankMoveSpeed = 5.f;
	posDelta *= tankMoveSpeed * getSpeedMultiplier();

	if(m_health <= 0 && m_mode != PlayerMode::SPECTATOR)
		return;

	sf::RectangleShape nextShape(logicalDimensions);
	nextShape.setPosition(m_pos + posDelta);

	if(map.isColliding(nextShape))
	{
		takeDamage(10);
		return;
	}

	m_pos += posDelta;

	float const angRad = std::atan2(posDelta.x, -posDelta.y);
	m_rot = sf::radians(angRad);
}

void PlayerState::setPosition(const sf::Vector2f &pos)
{
	m_pos = pos;
}

void PlayerState::setRotation(sf::Angle rot)
{
	m_rot = rot;
}

void PlayerState::takeDamage(int const amount)
{
	int remainingDamage = amount;

	// check for shield powerup
	for(auto &powerup : m_powerups)
	{
		if(powerup.type == PowerupType::SHIELD && powerup.value > 0)
		{
			int shieldAbsorbed = std::min(remainingDamage, powerup.value);
			powerup.value -= shieldAbsorbed;
			remainingDamage -= shieldAbsorbed;

			// deactivate shield if depleted
			if(powerup.value <= 0)
				powerup.deactivate();

			break;
		}
	}

	if(remainingDamage > 0)
	{
		m_health = std::max(0, m_health - remainingDamage);
	}
}

void PlayerState::heal(int const amount)
{
	m_health = std::max(m_maxHealth, m_health + amount);
}

void PlayerState::die()
{
	if(m_mode == PlayerMode::NORMAL)
		m_health = 0;
}

void PlayerState::revive()
{
	m_health = m_maxHealth;
}

bool PlayerState::canShoot() const
{
	return m_health > 0 && m_shootCooldown.isReady();
}

void PlayerState::shoot()
{
	if(canShoot())
	{
		float cooldown = getShootCooldown();
		m_shootCooldown = Cooldown(cooldown);
		m_shootCooldown.trigger();
	}
}

void PlayerState::applyPowerup(PowerupType type)
{
	for(auto &powerup : m_powerups)
	{
		if(!powerup.isActive())
		{
			powerup.apply(type);

			// apply instant effects
			if(type == PowerupType::HEALTH_PACK)
			{
				heal(PowerupConstants::HEALTH_PACK_HEAL);
			}
			return;
		}
	}

	// all slots full -> replace the first one
	m_powerups[0].apply(type);
	if(type == PowerupType::HEALTH_PACK)
	{
		heal(PowerupConstants::HEALTH_PACK_HEAL);
	}
}

bool PlayerState::hasPowerup(PowerupType type) const
{
	for(const auto &powerup : m_powerups)
	{
		if(powerup.type == type && powerup.isActive())
			return true;
	}
	return false;
}

const PowerupEffect *PlayerState::getPowerup(PowerupType type) const
{
	for(const auto &powerup : m_powerups)
	{
		if(powerup.type == type && powerup.isActive())
			return &powerup;
	}
	return nullptr;
}

float PlayerState::getSpeedMultiplier() const
{
	if(hasPowerup(PowerupType::SPEED_BOOST))
		return PowerupConstants::SPEED_BOOST_MULTIPLIER;
	return 1.f;
}

int PlayerState::getDamageMultiplier() const
{
	if(hasPowerup(PowerupType::DAMAGE_BOOST))
		return PowerupConstants::DAMAGE_BOOST_MULTIPLIER;
	return 1;
}

float PlayerState::getShootCooldown() const
{
	if(hasPowerup(PowerupType::RAPID_FIRE))
		return PowerupConstants::RAPID_FIRE_COOLDOWN;
	return GameConfig::Player::SHOOT_COOLDOWN;
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
	return m_health > 0 || m_mode == PlayerMode::SPECTATOR;
}

bool PlayerState::addToInventory(PowerupType type)
{
	for(size_t i = 0; i < m_inventory.size(); ++i)
	{
		if(m_inventory[i] == PowerupType::NONE)
		{
			m_inventory[i] = type;
			return true;
		}
	}
	return false;
}

void PlayerState::useSelectedItem()
{
	if(m_selectedSlot < 0 || m_selectedSlot >= static_cast<int>(m_inventory.size()))
		return;

	PowerupType itemType = m_inventory[m_selectedSlot];
	if(itemType == PowerupType::NONE)
		return;

	applyPowerup(itemType);

	m_inventory[m_selectedSlot] = PowerupType::NONE;
}

void PlayerState::setSelectedSlot(int slot)
{
	if(slot >= 0 && slot < static_cast<int>(m_inventory.size()))
		m_selectedSlot = slot;
}

PowerupType PlayerState::getInventoryItem(int slot) const
{
	if(slot >= 0 && slot < static_cast<int>(m_inventory.size()))
		return m_inventory[slot];
	return PowerupType::NONE;
}

void PlayerState::serialize(sf::Packet &pkt) const
{
	pkt << m_id;
	pkt << m_name;
	pkt << m_pos;
	pkt << m_rot;
	pkt << m_health;
	pkt << m_maxHealth;
	pkt << m_shootCooldown.getRemaining();
	pkt << m_kills;
	pkt << m_deaths;

	for(const auto &powerup : m_powerups)
	{
		powerup.serialize(pkt);
	}

	for(const auto &item : m_inventory)
	{
		pkt << static_cast<uint8_t>(item);
	}
	pkt << static_cast<int32_t>(m_selectedSlot);
}

void PlayerState::deserialize(sf::Packet &pkt)
{
	pkt >> m_id;
	pkt >> m_name;
	pkt >> m_pos;
	pkt >> m_rot;
	pkt >> m_health;
	pkt >> m_maxHealth;
	pkt >> m_kills;
	pkt >> m_deaths;

	float cooldownRemaining;
	pkt >> cooldownRemaining;
	m_shootCooldown.setRemaining(cooldownRemaining);

	for(auto &powerup : m_powerups)
	{
		powerup.deserialize(pkt);
	}

	for(auto &item : m_inventory)
	{
		uint8_t itemType;
		pkt >> itemType;
		item = static_cast<PowerupType>(itemType);
	}
	int32_t selectedSlot;
	pkt >> selectedSlot;
	m_selectedSlot = selectedSlot;

	// assert(m_health >= 0);
	// assert(m_maxHealth >= 0);
}

void PlayerState::setKills(uint32_t k)
{
	m_kills = k;
}
void PlayerState::setDeaths(uint32_t d)
{
	m_deaths = d;
}

uint32_t PlayerState::getKills() const
{
	return m_kills;
}
uint32_t PlayerState::getDeaths() const
{
	return m_deaths;
}

void PlayerState::addKill()
{
	m_kills++;
}
void PlayerState::addDeath()
{
	m_deaths++;
}