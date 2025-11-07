#pragma once
#include <vector>
#include <SFML/Network.hpp>
#include "Map/MapState.h"
#include "Player/PlayerState.h"
#include "Networking.h"

class WorldState
{
public:
	explicit WorldState(const MapState& map) : m_map(map) {}

	void update(float dt);
	void setPlayer(PlayerState &p);

	std::array<PlayerState, MAX_PLAYERS> &getPlayers();
	const MapState &map() const;
	MapState &map();

	// serialization (full snapshot)
	void serialize(sf::Packet &pkt) const;
	void deserialize(sf::Packet &pkt);

private:
	MapState m_map;
	// Players are not removed on disconnect,
	// as others can't join in the battle phase anyway.
	std::array<PlayerState, MAX_PLAYERS> m_players;
};
