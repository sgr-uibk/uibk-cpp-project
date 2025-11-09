#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <cstdint>
#include "../Powerup.h"

class ItemState
{
public:
	ItemState();
	ItemState(uint32_t id, sf::Vector2f position, PowerupType type);

	void update(float dt);

	uint32_t getId() const { return m_id; }
	sf::Vector2f getPosition() const { return m_position; }
	PowerupType getType() const { return m_type; }
	bool isActive() const { return m_active; }

	void setActive(bool active) { m_active = active; }
	void collect() { m_active = false; }

	sf::FloatRect getBounds() const;

	void serialize(sf::Packet& packet) const;
	void deserialize(sf::Packet& packet);

private:
	uint32_t m_id;
	sf::Vector2f m_position;
	PowerupType m_type;
	bool m_active;

	static constexpr float ITEM_SIZE = 24.f;  // collision box size
};
