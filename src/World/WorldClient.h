#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include "WorldState.h"
#include "Game/InterpClient.h"
#include "Player/PlayerClient.h"
#include "Map/MapClient.h"
#include "Projectile/ProjectileClient.h"
#include "Item/ItemClient.h"
#include "PauseMenuClient.h"
#include "ItemBarClient.h"
#include "HealthBar.h"

class WorldClient
{
  public:
	WorldClient(sf::RenderWindow &window, EntityId ownPlayerId, int mapIndex,
	            std::array<PlayerState, MAX_PLAYERS> players);

	std::optional<sf::Packet> update(sf::Vector2f posDelta);
	void draw(sf::RenderWindow &window) const;
	[[nodiscard]] PlayerState getOwnPlayerState() const
	{
		return m_players[m_ownPlayerId - 1].getState();
	}

	void applyNonInterpState(WorldState const &snapshot);
	WorldState &getState();
	void pollEvents();

	bool m_bAcceptInput;
	sf::Clock m_frameClock;
	sf::Clock m_tickClock;
	PauseMenuClient m_pauseMenu;
	Tick m_clientTick = 0;

  private:
	void propagateUpdate(float dt);
	void handleResize(sf::Vector2u newSize);
	WorldState m_state;
	ItemBarClient m_itemBar;
	HealthBar m_healthBar;
	sf::RenderWindow &m_window;
	MapClient m_mapClient;
	std::array<PlayerClient, MAX_PLAYERS> m_players;
	std::vector<ProjectileClient> m_projectiles;
	std::vector<ItemClient> m_items;
	EntityId m_ownPlayerId;
	sf::View m_worldView;
	sf::View m_hudView;
	float m_zoomLevel = 1.0f;

  public:
	InterpClient m_interp;
};
