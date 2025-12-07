#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <memory>
#include <optional>
#include <vector>
#include <array>

#include "Networking.h"
#include "WorldState.h"
#include "Map/MapClient.h"
#include "Player/PlayerClient.h"
#include "Game/InterpClient.h"
#include "Item/ItemClient.h"
#include "Projectile/ProjectileClient.h"
#include "World/PauseMenuClient.h"
#include "UI/Scoreboard.h"
#include "ItemBarClient.h"

class WorldClient
{
  public:
	WorldClient(sf::RenderWindow &window, EntityId ownPlayerId, std::array<PlayerState, MAX_PLAYERS> &players);
	~WorldClient();

	std::optional<sf::Packet> update(sf::Vector2f posDelta);
	void draw(sf::RenderWindow &window) const;
	[[nodiscard]] PlayerState getOwnPlayerState() const
	{
		return m_players[m_ownPlayerId - 1].getState();
	}
	void pollEvents();

	void applyNonInterpState(WorldState const &snapshot);
	WorldState &getState();

	void showScoreboard(EntityId winnerId, std::vector<struct Scoreboard::PlayerStats> const &playerStats);

	bool isReturnToLobbyRequested() const
	{
		return m_returnToLobby;
	}

	void handleResize(sf::Vector2u newSize);

	Tick m_clientTick = 0;

  private:
	void propagateUpdate(float dt);
	// Member order matches initialization order in constructor
	bool m_bAcceptInput;

  public:
	PauseMenuClient m_pauseMenu;

  private:
	WorldState m_state;
	ItemBarClient m_itemBar;
	sf::RenderWindow &m_window;
	MapClient m_mapClient;
	std::array<PlayerClient, MAX_PLAYERS> m_players;
	EntityId m_ownPlayerId;

  public:
	InterpClient m_interp;

  private:
	std::vector<ProjectileClient> m_projectiles;
	std::vector<ItemClient> m_items;

	sf::Clock m_frameClock;
	sf::Clock m_tickClock;
	sf::View m_worldView;
	sf::View m_hudView;
	float m_zoomLevel = 1.0f;

	std::unique_ptr<Scoreboard> m_scoreboard;
	bool m_returnToLobby = false;
};