#include <algorithm>
#include "WorldState.h"

#include <spdlog/spdlog.h>

WorldState::WorldState() : m_map(WINDOW_DIMf), m_players()
{
}

WorldState::WorldState(sf::Vector2f mapSize) : m_map(mapSize), m_players()
{
}

void WorldState::update()
{
	for(auto &p : m_players)
		p.update();
}

void WorldState::setPlayer(PlayerState const &p)
{
	size_t const idx = p.m_id - 1;
	m_players[idx] = p;
}

std::array<PlayerState, MAX_PLAYERS> &WorldState::getPlayers()
{
	return m_players;
}

PlayerState &WorldState::getPlayerById(size_t id)
{
	if(id == 0)
		SPDLOG_ERROR("ID 0 is reserved for the server!");
	return m_players[id - 1];
}

MapState const &WorldState::getMap() const
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

// The map is static, it's not serialized in snapshots, therefore don't assign it.
// TODO: For "Dynamic Map elements": Make m_map mutable, included map in snapshots, then remove this operator
WorldState &WorldState::operator=(const WorldState &other)
{
	this->m_players = other.m_players;
	return *this;
}
