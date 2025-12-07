#pragma once
#include "Networking.h"
#include "RingBuffer.h"
#include "Player/PlayerClient.h"
#include "World/WorldState.h"

constexpr float EXPECTED_RTT_S = 0.1f;
// How many inputs can be stored in between server ticks
constexpr size_t CLIENT_INPUT_RB_DEPTH = 8; // EXPECTED_RTT_S * RENDER_TICK_HZ;
// How many snapshots are stored for interpolation
constexpr size_t CLIENT_SNAP_RB_DEPTH = 4;

class InterpClient
{
  public:
	InterpClient(MapState const &map, EntityId id, std::array<PlayerClient, MAX_PLAYERS> &players);
	void predictMovement(sf::Vector2f posDelta);
	sf::Vector2f getCumulativeInputs(Tick clientTick);
	bool storeSnapshot(Tick authTick, sf::Packet snapPkt, WorldState const &ws);
	void correctLocalPlayer();
	void interpolateEnemies();

  private:
	void replayInputs(PlayerState &player);
	void replayInputs(PlayerClient &player);
	MapState const &m_map;
	size_t const m_id;
	std::array<PlayerClient, MAX_PLAYERS> &m_players;
	PlayerClient &m_player = m_players[m_id - 1];

	Tick m_ackedTick = 0;      // our last tick that the server has seen
	sf::Vector2f m_inputAcc{}; // Inputs that are not yet in-flight
	RingQueue<Ticked<sf::Vector2f>, CLIENT_INPUT_RB_DEPTH> m_inflightInputs;
	RingQueue<Ticked<std::array<PlayerState, MAX_PLAYERS>>, CLIENT_SNAP_RB_DEPTH> m_stateBuffer;
};
