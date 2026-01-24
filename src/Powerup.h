#pragma once
#include "GameConfig.h"
#include <cstdint>
#include <SFML/Network.hpp>

enum class PowerupType : uint8_t
{
	NONE = 0,
	HEALTH_PACK,
	SPEED_BOOST,
	DAMAGE_BOOST,
	SHIELD,
	RAPID_FIRE
};

struct PowerupEffect
{
	PowerupType type = PowerupType::NONE;
	float duration = 0.f;
	int value = 0; // shield health remaining

	void update(float dt)
	{
		duration = std::max(0.f, duration - dt);
	}

	bool isActive() const
	{
		return duration > 0.f || (type == PowerupType::SHIELD && value > 0);
	}

	void deactivate()
	{
		duration = 0.f;
		value = 0;
		type = PowerupType::NONE;
	}

	void apply(PowerupType newType)
	{
		type = newType;

		switch(type)
		{
		case PowerupType::SHIELD:
			duration = GameConfig::Powerup::EFFECT_DURATION;
			value = GameConfig::Powerup::SHIELD_HP;
			break;

		case PowerupType::HEALTH_PACK:
			duration = 0.f;
			value = 0;
			break;

		case PowerupType::SPEED_BOOST:
		case PowerupType::DAMAGE_BOOST:
		case PowerupType::RAPID_FIRE:
			duration = GameConfig::Powerup::EFFECT_DURATION;
			value = 0;
			break;

		case PowerupType::NONE:
			deactivate();
			break;
		}
	}

	void serialize(sf::Packet &packet) const
	{
		packet << static_cast<uint8_t>(type);
		packet << duration << static_cast<int32_t>(value);
	}

	void deserialize(sf::Packet &packet)
	{
		uint8_t typeVal;
		int32_t valueVal;
		packet >> typeVal;
		packet >> duration;
		packet >> valueVal;
		type = static_cast<PowerupType>(typeVal);
		value = valueVal;
	}
};
