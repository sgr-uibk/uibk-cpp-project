#include "Player/PlayerState.h"
#include "Map/MapState.h"
#include "Networking.h"
#include <algorithm>
#include <spdlog/spdlog.h>

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
		powerup.update(dt);
	}
}

void PlayerState::moveOn(MapState const &map, sf::Vector2f posDelta)
{
	if(m_health <= 0)
		return;

	constexpr float tankMoveSpeed = 5.f;
	posDelta *= tankMoveSpeed * getSpeedMultiplier();
	sf::RectangleShape nextShape(logicalDimensions);
	nextShape.setPosition(m_iState.m_pos + posDelta);

	if(map.isColliding(nextShape))
	{ // player crashed against something, deal them damage
		takeDamage(10);
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
				powerup.deactivate();

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
				heal(GameConfig::Powerup::HEALTH_PACK_HEAL);
			}
			return;
		}
	}

	// all slots full -> replace the first one
	m_powerups[0].apply(type);
	if(type == PowerupType::HEALTH_PACK)
	{
		heal(GameConfig::Powerup::HEALTH_PACK_HEAL);
	}
}

bool PlayerState::hasPowerup(PowerupType type) const
{
	for(auto const &powerup : m_powerups)
	{
		if(powerup.type == type && powerup.isActive())
			return true;
	}
	return false;
}

PowerupEffect const *PlayerState::getPowerup(PowerupType type) const
{
	for(auto const &powerup : m_powerups)
	{
		if(powerup.type == type && powerup.isActive())
			return &powerup;
	}
	return nullptr;
}

float PlayerState::getSpeedMultiplier() const
{
	if(hasPowerup(PowerupType::SPEED_BOOST))
		return GameConfig::Powerup::SPEED_BOOST_MULTIPLIER;
	return 1.f;
}

int PlayerState::getDamageMultiplier() const
{
	if(hasPowerup(PowerupType::DAMAGE_BOOST))
		return GameConfig::Powerup::DAMAGE_BOOST_MULTIPLIER;
	return 1;
}

float PlayerState::getShootCooldown() const
{
	if(hasPowerup(PowerupType::RAPID_FIRE))
		return GameConfig::Powerup::RAPID_FIRE_COOLDOWN;
	return GameConfig::Player::SHOOT_COOLDOWN;
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

void PlayerState::useItem(size_t const slot)
{
	if(slot >= m_inventory.size())
		return;

	PowerupType itemType = m_inventory[slot];
	if(itemType == PowerupType::NONE)
		return;

	applyPowerup(itemType);
	m_inventory[slot] = PowerupType::NONE;
}

PowerupType PlayerState::getInventoryItem(int slot) const
{
	if(slot >= 0 && slot < static_cast<int>(m_inventory.size()))
		return m_inventory[slot];
	return PowerupType::NONE;
}

void PlayerState::serialize(sf::Packet &pkt) const
{
	pkt << m_iState << m_health << m_maxHealth;
	pkt << m_shootCooldown.getRemaining();

	for(auto const &powerup : m_powerups)
	{
		powerup.serialize(pkt);
	}

	for(auto const &item : m_inventory)
	{
		pkt << static_cast<uint8_t>(item);
	}
}

void PlayerState::deserialize(sf::Packet &pkt)
{
	pkt >> m_iState >> m_health >> m_maxHealth;

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
}
