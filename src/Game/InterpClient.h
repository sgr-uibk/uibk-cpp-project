#pragma once
#include "Networking.h"
#include "RingBuffer.h"
#include "Player/PlayerClient.h"
#include "World/WorldState.h"

constexpr size_t roundup_pow2(size_t const x)
{
	size_t p = 1;
	while(p < x)
		p <<= 1;
	return p;
}

// How many inputs can be stored in between server ticks
constexpr size_t CLIENT_INPUT_RB_DEPTH = roundup_pow2(RENDER_TICK_HZ / UNRELIABLE_TICK_HZ);
// How many snapshots are stored for interpolation
constexpr size_t CLIENT_SNAP_RB_DEPTH = 4;

class InterpClient
{
  public:
	InterpClient(MapState const &map, EntityId id, std::array<PlayerClient, MAX_PLAYERS> &players);
	void predictMovement(sf::Vector2f posDelta);
	sf::Vector2f getCumulativeInputs(Tick clientTick);
	WorldState *storeSnapshot(Tick authTick, sf::Packet snapPkt);
	void syncLocalPlayer(WorldState const &snap);
	void interpolateEnemies();

  private:
	void replayInputs();
	MapState const &m_map;
	size_t m_id;
	std::array<PlayerClient, MAX_PLAYERS> &m_players;
	PlayerClient &m_player = m_players[m_id - 1];

	Tick m_ackedTick = 0; // last our last tick that the server has seen
	sf::Vector2f m_inputAcc{}; // Inputs that are not yet in-flight
	RingQueue<Ticked<sf::Vector2f>, CLIENT_INPUT_RB_DEPTH> m_inflightInputs;
	RingQueue<Ticked<WorldState>, CLIENT_SNAP_RB_DEPTH> m_snapshotBuffer;
};
