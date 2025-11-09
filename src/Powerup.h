#pragma once
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
	float maxDuration = 5.f;
	int value = 0; // shield health remaining

	void update(float dt)
	{
		if(duration > 0.f)
		{
			duration -= dt;
			if(duration < 0.f)
				duration = 0.f;
		}
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
		case PowerupType::HEALTH_PACK:
			duration = 0.f;
			value = 50; // heal amount
			break;

		case PowerupType::SPEED_BOOST:
			duration = maxDuration;
			value = 0;
			break;

		case PowerupType::DAMAGE_BOOST:
			duration = maxDuration;
			value = 0;
			break;

		case PowerupType::SHIELD:
			duration = 0.f;
			value = 50; // shield health
			break;

		case PowerupType::RAPID_FIRE:
			duration = maxDuration;
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
		packet << duration;
		packet << static_cast<int32_t>(value);
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

namespace PowerupConstants
{
constexpr float SPEED_BOOST_MULTIPLIER = 1.5f;
constexpr int DAMAGE_BOOST_MULTIPLIER = 2;
constexpr float RAPID_FIRE_COOLDOWN = 0.1f;
constexpr float POWERUP_DURATION = 5.f;
constexpr int HEALTH_PACK_HEAL = 50;
constexpr int SHIELD_HP = 50;
} // namespace PowerupConstants
