#include "Player/PlayerState.h"
#include "Map/MapState.h"
#include "Networking.h"
#include <algorithm>
#include <spdlog/spdlog.h>

using namespace GameConfig::Player;
using namespace GameConfig::Powerup;

void InterpPlayerState::overwriteBy(InterpPlayerState const auth)
{
	*this = std::move(auth);
}
sf::Packet &operator>>(sf::Packet &pkt, InterpPlayerState &obj)
{
	pkt >> obj.m_pos >> obj.m_rot >> obj.m_cannonRot;
	return pkt;
}

sf::Packet &operator<<(sf::Packet &pkt, InterpPlayerState const &obj)
{
	pkt << obj.m_pos << obj.m_rot << obj.m_cannonRot;
	return pkt;
}

PlayerState::PlayerState(EntityId id, sf::Vector2f pos, sf::Angle rot, int maxHealth)
	: m_id(id), m_name(""), m_iState(pos, rot, rot), m_maxHealth(maxHealth)
{
}
PlayerState::PlayerState(EntityId id, InterpPlayerState ips, int maxHealth)
	: m_id(id), m_iState(ips), m_maxHealth(maxHealth)
{
}

void PlayerState::assignSnappedState(PlayerState const &other)
{
	auto const iS = this->m_iState;
	*this = other;
	this->m_iState = iS;
}

PlayerState::PlayerState(EntityId id, sf::Vector2f pos, int maxHealth) : PlayerState(id, pos, sf::degrees(0), maxHealth)
{
}

PlayerState::PlayerState(EntityId id, sf::Packet &pkt) : m_id(id)
{
	deserialize(pkt);
}

void PlayerState::update(float dt)
{
	// cooldowns
	m_shootCooldown.update(dt);

	for(auto &powerup : m_powerups)
	{
		if(powerup.type == PowerupType::NONE)
			continue;

		PowerupType powerupType = powerup.type;
		bool wasActive = powerup.isActive();

		powerup.update(dt);

		bool isActive = powerup.isActive();

		if(wasActive && !isActive)
		{
			startReuseCooldown(powerupType);
			powerup.type = PowerupType::NONE;
		}
	}

	for(auto &cooldown : m_powerupReuseCooldowns)
	{
		cooldown.update(dt);
	}
}

void PlayerState::startReuseCooldown(PowerupType type)
{
	if(type == PowerupType::NONE)
		return;

	int idx = static_cast<int>(type);
	if(idx > 0 && idx < NUM_POWERUP_TYPES)
	{
		m_powerupReuseCooldowns[idx] = Cooldown(GameConfig::UI::POWERUP_REUSE_COOLDOWN);
		m_powerupReuseCooldowns[idx].trigger();
	}
}

sf::Vector2f PlayerState::moveOn(MapState const &map, sf::Vector2f posDelta)
{
	if(m_health <= 0)
		return {0, 0};

	constexpr float tankMoveSpeed = 5.f;
	posDelta *= tankMoveSpeed * getSpeedMultiplier();
	sf::RectangleShape nextShape(logicalDimensions);
	nextShape.setPosition(m_iState.m_pos + posDelta);

	if(map.isColliding(nextShape))
	{                                            // player crashed against something, deal them damage
		takeDamage(ceilf(getSpeedMultiplier())); // impact damage prop. speed
		posDelta = {0, 0};
	}
	else
	{ // no collision, let them move there
		m_iState.m_pos += posDelta;
		float lengthSq = posDelta.lengthSquared();
		if(lengthSq > 0.0001f)
		{
			m_iState.m_rot = sf::Vector2f(1.f, 1.f).angleTo(posDelta);
		}
	}
	return posDelta;
}

void PlayerState::setRotation(sf::Angle rot)
{
	m_iState.m_rot = rot;
}

void PlayerState::takeDamage(int amount)
{
	// check for shield powerup
	for(auto &powerup : m_powerups)
	{
		if(powerup.type == PowerupType::SHIELD && powerup.value > 0)
		{
			int shieldAbsorbed = std::min(amount, powerup.value);
			powerup.value -= shieldAbsorbed;
			amount -= shieldAbsorbed;

			// deactivate shield if depleted
			if(powerup.value <= 0)
			{
				startReuseCooldown(PowerupType::SHIELD);
				powerup.deactivate();
			}
			break;
		}
	}

	if(amount > 0)
	{
		m_health = std::max(0, m_health - amount);
	}
}

void PlayerState::heal(int const amount)
{
	m_health = std::min(m_maxHealth, m_health + amount);
}

void PlayerState::die()
{
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

bool PlayerState::tryShoot()
{
	return m_health > 0 && m_shootCooldown.try_trigger(getShootCooldown());
}

bool PlayerState::canUsePowerup(PowerupType type) const
{
	if(type == PowerupType::NONE)
		return false;

	int idx = static_cast<int>(type);
	if(idx <= 0 || idx >= NUM_POWERUP_TYPES)
		return false;

	if(hasPowerup(type))
		return false;

	return m_powerupReuseCooldowns[idx].isReady();
}

float PlayerState::getPowerupReuseCooldown(PowerupType type) const
{
	if(type == PowerupType::NONE)
		return 0.f;

	int idx = static_cast<int>(type);
	if(idx <= 0 || idx >= NUM_POWERUP_TYPES)
		return 0.f;

	return m_powerupReuseCooldowns[idx].getRemaining();
}

void PlayerState::applyPowerup(PowerupType type)
{
	if(type == PowerupType::HEALTH_PACK)
	{
		heal(GameConfig::Powerup::HEALTH_PACK_HEAL);
		startReuseCooldown(type);
		return;
	}

	for(auto &powerup : m_powerups)
	{
		if(powerup.type == PowerupType::NONE)
		{
			powerup.apply(type);
			return;
		}
	}

	m_powerups[0].apply(type);
}

bool PlayerState::hasPowerup(PowerupType type) const
{
	return std::any_of(m_powerups.begin(), m_powerups.end(),
	                   [type](auto const &p) { return p.type == type && p.isActive(); });
}

PowerupEffect const *PlayerState::getPowerup(PowerupType type) const
{
	auto it = std::find_if(m_powerups.begin(), m_powerups.end(),
	                       [type](auto const &p) { return p.type == type && p.isActive(); });
	return it != m_powerups.end() ? &(*it) : nullptr;
}

float PlayerState::getSpeedMultiplier() const
{
	if(hasPowerup(PowerupType::SPEED_BOOST))
		return SPEED_BOOST_MULTIPLIER;
	return 1.f;
}

int PlayerState::getDamageMultiplier() const
{
	if(hasPowerup(PowerupType::DAMAGE_BOOST))
		return DAMAGE_BOOST_MULTIPLIER;
	return 1;
}

float PlayerState::getShootCooldown() const
{
	if(hasPowerup(PowerupType::RAPID_FIRE))
		return RAPID_FIRE_COOLDOWN;
	return SHOOT_COOLDOWN;
}

uint32_t PlayerState::getPlayerId() const
{
	return m_id;
}

sf::Vector2f PlayerState::getPosition() const
{
	return m_iState.m_pos;
}

sf::Angle PlayerState::getRotation() const
{
	return m_iState.m_rot;
}

sf::Angle PlayerState::getCannonRotation() const
{
	return m_iState.m_cannonRot;
}

void PlayerState::setCannonRotation(sf::Angle angle)
{
	m_iState.m_cannonRot = angle;
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

bool PlayerState::useItem(size_t const slot)
{
	if(slot == 0 || slot > m_inventory.size())
		return false;

	size_t const idx = slot - 1;
	PowerupType itemType = m_inventory[idx];
	if(itemType == PowerupType::NONE)
		return false;

	if(!canUsePowerup(itemType))
		return false;

	applyPowerup(itemType);
	m_inventory[idx] = PowerupType::NONE;
	return true;
}

PowerupType PlayerState::getInventoryItem(int slot) const
{
	if(slot >= 0 && slot < static_cast<int>(m_inventory.size()))
		return m_inventory[slot];
	return PowerupType::NONE;
}

void PlayerState::serialize(sf::Packet &pkt) const
{
	pkt << m_name << m_iState << m_health << m_maxHealth;
	pkt << m_shootCooldown.getRemaining();

	for(auto const &powerup : m_powerups)
	{
		powerup.serialize(pkt);
	}

	for(int i = 0; i < NUM_POWERUP_TYPES; ++i)
	{
		pkt << m_powerupReuseCooldowns[i].getRemaining();
	}

	for(auto const &item : m_inventory)
	{
		pkt << static_cast<uint8_t>(item);
	}
}

void PlayerState::deserialize(sf::Packet &pkt)
{
	pkt >> m_name >> m_iState >> m_health >> m_maxHealth;

	float cooldownRemaining;
	pkt >> cooldownRemaining;
	m_shootCooldown.setRemaining(cooldownRemaining);

	for(auto &powerup : m_powerups)
	{
		powerup.deserialize(pkt);
	}

	for(int i = 0; i < NUM_POWERUP_TYPES; ++i)
	{
		float remaining;
		pkt >> remaining;
		if(remaining > 0.f)
		{
			m_powerupReuseCooldowns[i] = Cooldown(GameConfig::UI::POWERUP_REUSE_COOLDOWN);
			m_powerupReuseCooldowns[i].setRemaining(remaining);
		}
	}

	for(auto &item : m_inventory)
	{
		uint8_t itemType;
		pkt >> itemType;
		item = static_cast<PowerupType>(itemType);
	}
}
