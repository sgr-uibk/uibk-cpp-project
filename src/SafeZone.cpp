#include "SafeZone.h"
#include <algorithm>
#include <cmath>

void SafeZone::update(float const dt)
{
	if(!isActive || currentRadius <= targetRadius)
		return;

	currentRadius = std::max(currentRadius - shrinkSpeed * dt, targetRadius);
}

bool SafeZone::isOutside(sf::Vector2f const position) const
{
	if(!isActive)
		return false;

	sf::Vector2f const delta = position - center;
	float const distSq = delta.x * delta.x + delta.y * delta.y;
	return distSq > currentRadius * currentRadius;
}

void SafeZone::serialize(sf::Packet &pkt) const
{
	pkt << isActive << center.x << center.y << currentRadius << targetRadius << shrinkSpeed << damagePerSecond;
}

void SafeZone::deserialize(sf::Packet &pkt)
{
	pkt >> isActive >> center.x >> center.y >> currentRadius >> targetRadius >> shrinkSpeed >> damagePerSecond;
}

sf::Packet &operator<<(sf::Packet &pkt, SafeZone const &zone)
{
	zone.serialize(pkt);
	return pkt;
}

sf::Packet &operator>>(sf::Packet &pkt, SafeZone &zone)
{
	zone.deserialize(pkt);
	return pkt;
}
