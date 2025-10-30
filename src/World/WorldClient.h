#pragma once
#include <SFML/Graphics.hpp>
#include "WorldState.h"
#include "Player/PlayerClient.h"
#include "Map/MapClient.h"

class WorldClient
{
public:
	explicit WorldClient(sf::Vector2f dimensions, EntityId ownPlayerId, std::array<PlayerClient, MAX_PLAYERS> &players);

	sf::Packet update(float dt);
	void draw(sf::RenderWindow &window) const;

	[[nodiscard]] PlayerState getOwnPlayerState()
	{
		return m_players[m_ownPlayerId-1].getState();
		//return m_state.getPlayers()[m_ownPlayerId-1];
	}

	void applyServerSnapshot(const WorldState &snapshot);
	WorldState &getState();

private:
	sf::Clock m_tickClock;
	WorldState m_state;
	MapClient m_mapClient;
	std::array<PlayerClient, MAX_PLAYERS> m_players;
	EntityId m_ownPlayerId;
	sf::View m_worldView;
	sf::View m_hudView;
};
