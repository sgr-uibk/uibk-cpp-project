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

const std::array<PlayerState, MAX_PLAYERS> &WorldState::getPlayers() const
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
	for (const auto &p : m_players)
    {
        if (p.m_id == 0)
            continue;

        p.serialize(pkt);
    }
}

void WorldState::deserialize(sf::Packet &pkt)
{
	while (!pkt.endOfPacket())
    {
        PlayerState temp;
        temp.deserialize(pkt);

        if (temp.m_id == 0)
            continue;

        m_players[temp.m_id - 1] = temp;
    }
}
