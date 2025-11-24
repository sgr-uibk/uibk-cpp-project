#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include "WorldState.h"
#include "Player/PlayerClient.h"
#include "Map/MapClient.h"
#include "Projectile/ProjectileClient.h"
#include "Item/ItemClient.h"

class Scoreboard;

class WorldClient
{
  public:
	struct PlayerStats
	{
		EntityId id;
		std::string name;
		int kills;
		int deaths;
	};

	WorldClient(sf::RenderWindow &window, EntityId ownPlayerId, std::array<PlayerState, MAX_PLAYERS> &players,
	            sf::Music *battleMusic = nullptr);
	~WorldClient();

	std::optional<sf::Packet> update();
	void draw(sf::RenderWindow &window) const;

	[[nodiscard]] PlayerState getOwnPlayerState() const
	{
		return m_players[m_ownPlayerId - 1].getState();
	}

	void applyServerSnapshot(const WorldState &snapshot);
	WorldState &getState();
	void pollEvents();
	void handleResize(sf::Vector2u newSize);

	bool isPaused() const
	{
		return m_isPaused;
	}
	bool shouldDisconnect() const
	{
		return m_shouldDisconnect;
	}
	bool shouldReturnToLobby() const
	{
		return m_shouldReturnToLobby;
	}

	void showScoreboard(EntityId winnerId, const std::vector<PlayerStats> &playerStats = {});

	bool m_bAcceptInput;

	sf::Clock m_frameClock;
	sf::Clock m_tickClock;

  private:
	void drawPauseMenu(sf::RenderWindow &window) const;
	void handlePauseMenuClick(sf::Vector2f mousePos);
	void drawHotbar(sf::RenderWindow &window) const;

	sf::RenderWindow &m_window;

	WorldState m_state;
	MapClient m_mapClient;
	std::array<PlayerClient, MAX_PLAYERS> m_players;
	std::vector<ProjectileClient> m_projectiles;
	std::vector<ItemClient> m_items;
	EntityId m_ownPlayerId;
	sf::View m_worldView;
	sf::View m_hudView;

	enum class PauseMenuState
	{
		MAIN,
		SETTINGS
	};
	bool m_isPaused = false;
	bool m_shouldDisconnect = false;
	bool m_shouldReturnToLobby = false;
	PauseMenuState m_pauseMenuState = PauseMenuState::MAIN;
	sf::Font m_pauseFont;
	std::vector<sf::RectangleShape> m_pauseButtons;
	std::vector<sf::Text> m_pauseButtonTexts;

	std::unique_ptr<Scoreboard> m_scoreboard;
	bool m_slotChangeRequested = false;
	int m_requestedSlot = 0;
	bool m_useItemRequested = false;

	sf::Music *m_battleMusic = nullptr;
	std::optional<sf::Sound> m_shootSound;
};
