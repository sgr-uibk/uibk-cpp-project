#pragma once
#include <SFML/System/Vector2.hpp>
#include <SFML/Network/Packet.hpp>

struct SafeZone
{
	sf::Vector2f center;
	float currentRadius = 0.f;
	float targetRadius = 0.f;
	float shrinkSpeed = 0.f; // units per second
	float damagePerSecond = 0.f;
	bool isActive = false;

	void update(float dt);
	bool isOutside(sf::Vector2f position) const;

	void serialize(sf::Packet &pkt) const;
	void deserialize(sf::Packet &pkt);
};

sf::Packet &operator<<(sf::Packet &pkt, SafeZone const &zone);
sf::Packet &operator>>(sf::Packet &pkt, SafeZone &zone);
