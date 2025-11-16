#pragma once
#include <SFML/Network.hpp>
#include "Map/MapState.h"
#include "Player/PlayerState.h"
#include "Networking.h"

class WorldState
{
  public:
	WorldState();
	explicit WorldState(sf::Vector2f mapSize);
	void update(float dt);
	void setPlayer(PlayerState const &p);

	std::array<PlayerState, MAX_PLAYERS> &getPlayers();
	PlayerState &getPlayerById(size_t id);
	[[nodiscard]] MapState const &getMap() const;

	// serialization (full snapshot)
	void serialize(sf::Packet &pkt) const;
	void deserialize(sf::Packet &pkt);

	WorldState &operator=(WorldState const &);

  private:
	MapState const m_map;
	// Players are not removed on disconnect,
	// as others can't join in the battle phase anyway.
	std::array<PlayerState, MAX_PLAYERS> m_players;
};
