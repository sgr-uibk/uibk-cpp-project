#include "ItemState.h"
#include "../Networking.h"

ItemState::ItemState() : m_id(0), m_position(0.f, 0.f), m_type(PowerupType::NONE), m_active(false)
{
}

ItemState::ItemState(uint32_t id, sf::Vector2f position, PowerupType type)
	: m_id(id), m_position(position), m_type(type), m_active(true)
{
}

void ItemState::update([[maybe_unused]] int32_t dt)
{
	// items currently don't need per-frame updates
}

sf::FloatRect ItemState::getBounds() const
{
	return sf::FloatRect(sf::Vector2f(m_position.x - ITEM_SIZE / 2.f, m_position.y - ITEM_SIZE / 2.f),
	                     sf::Vector2f(ITEM_SIZE, ITEM_SIZE));
}

void ItemState::serialize(sf::Packet &packet) const
{
	packet << m_id;
	packet << m_position;
	packet << static_cast<uint8_t>(m_type);
	packet << m_active;
}

void ItemState::deserialize(sf::Packet &packet)
{
	uint8_t typeVal;
	packet >> m_id;
	packet >> m_position;
	packet >> typeVal;
	packet >> m_active;
	m_type = static_cast<PowerupType>(typeVal);
}
