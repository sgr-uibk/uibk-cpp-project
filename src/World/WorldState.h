#pragma once
#include <SFML/Network.hpp>
#include "Map/MapState.h"
#include "Player/PlayerState.h"
#include "Networking.h"

struct WorldState
{
	WorldState();
	explicit WorldState(sf::Vector2f mapSize);
	void update();
	void setPlayer(PlayerState const &p);

	std::array<PlayerState, MAX_PLAYERS> &getPlayers();
	PlayerState &getPlayerById(size_t id);
	[[nodiscard]] MapState const &getMap() const;

	// serialization (full snapshot)
	void serialize(sf::Packet &pkt) const;
	void deserialize(sf::Packet &pkt);

	WorldState &operator=(WorldState const &);

	MapState const m_map;
	std::array<PlayerState, MAX_PLAYERS> m_players;
};
