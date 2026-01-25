#pragma once
#include <SFML/System/Vector2.hpp>
#include <SFML/Network/Packet.hpp>

struct SafeZone
{
	sf::Vector2f center;
	float currentRadius;
	float targetRadius;
	float shrinkSpeed;       // units per second
	float damagePerSecond;
	bool isActive = false;

	void update(float dt);
	bool isOutside(sf::Vector2f position) const;

	void serialize(sf::Packet &pkt) const;
	void deserialize(sf::Packet &pkt);
};

sf::Packet &operator<<(sf::Packet &pkt, SafeZone const &zone);
sf::Packet &operator>>(sf::Packet &pkt, SafeZone &zone);
