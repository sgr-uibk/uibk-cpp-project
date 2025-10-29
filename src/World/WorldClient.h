#pragma once
#include <SFML/Graphics.hpp>
#include "WorldState.h"
#include "Player/PlayerClient.h"
#include "Map/MapClient.h"

class WorldClient
{
public:
	WorldClient(sf::RenderWindow &window, EntityId ownPlayerId, std::array<PlayerState, MAX_PLAYERS> &players);

	sf::Packet update();
	void draw(sf::RenderWindow &window) const;

	[[nodiscard]] PlayerState getOwnPlayerState() const
	{
		return m_players[m_ownPlayerId - 1].getState();
	}

	void applyServerSnapshot(const WorldState &snapshot);
	WorldState &getState();
	void pollInputs();

	bool m_bAcceptInput;

	sf::Clock m_frameClock;
	sf::Clock m_tickClock;
private:
	sf::RenderWindow &m_window;

	WorldState m_state;
	MapClient m_mapClient;
	std::array<PlayerClient, MAX_PLAYERS> m_players;
	EntityId m_ownPlayerId;
	sf::View m_worldView;
	sf::View m_hudView;
};
