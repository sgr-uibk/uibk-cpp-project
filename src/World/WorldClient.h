#pragma once
#include <SFML/Graphics.hpp>
#include <deque>
#include "RingBuffer.h"
#include "WorldState.h"
#include "Player/PlayerClient.h"
#include "Map/MapClient.h"

class WorldClient
{
  public:
	WorldClient(sf::RenderWindow &window, EntityId ownPlayerId, std::array<PlayerState, MAX_PLAYERS> &players);

	std::optional<sf::Packet> update(sf::Vector2f vector2);
	void draw(sf::RenderWindow &window) const;

	[[nodiscard]] PlayerState getOwnPlayerState() const
	{
		return m_players[m_ownPlayerId - 1].getState();
	}

	void applyServerSnapshot(WorldState const &snapshot);
	void reconcileLocalPlayer(Tick serverTick, WorldState const &snap);
	void interpolateEnemies() const;

	WorldState &getState();
	void pollEvents();

	bool m_bAcceptInput;

	sf::Clock m_frameClock;
	sf::Clock m_tickClock;

	sf::Vector2f m_inputAcc = {0, 0}; // not yet in flight
	RingQueue<Ticked<sf::Vector2f>, 8> m_inflightInputs;
	sf::Vector2f m_pendingInput = {0, 0};
	RingQueue<Ticked<WorldState>, 8>
		m_snapshotBuffer;  // the top of the snapshot buffer is the latest tick recvd from the server
	Tick m_clientTick = 0; // prediction reference; what inputs need to be reapplied after reconciliation
	Tick m_authTick = 0;   // last authoritative tick received from server

  private:
	sf::RenderWindow &m_window;

	WorldState m_state;
	MapClient m_mapClient;
	std::array<PlayerClient, MAX_PLAYERS> m_players;
	EntityId m_ownPlayerId;
	sf::View m_worldView;
	sf::View m_hudView;
};
