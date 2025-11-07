#include <algorithm>
#include "WorldState.h"

void WorldState::update(float dt)
{
	for(auto &p : m_players)
		p.update(dt);
}

void WorldState::setPlayer(PlayerState &p)
{
	size_t const idx = p.m_id - 1;
	m_players[idx] = p;
}

std::array<PlayerState, MAX_PLAYERS> &WorldState::getPlayers()
{
	return m_players;
}

MapState &WorldState::map()
{
	return m_map;
}

const MapState &WorldState::map() const
{
	return m_map;
}

void WorldState::serialize(sf::Packet &pkt) const
{
	for(uint32_t i = 0; i < MAX_PLAYERS; ++i)
	{
		m_players[i].serialize(pkt);
	}
}

void WorldState::deserialize(sf::Packet &pkt)
{
	for(uint32_t i = 0; i < MAX_PLAYERS; ++i)
	{
		m_players[i].deserialize(pkt);
	}
}
