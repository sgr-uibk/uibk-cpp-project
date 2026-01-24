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
#include "PowerupCooldownPanelClient.h"
#include "AmmunitionDisplayClient.h"
#include "MinimapClient.h"

struct WorldUpdateData
{
	sf::Vector2f posDelta = {0, 0};
	sf::Angle rot = sf::radians(0);
	sf::Angle cannonRot = sf::radians(0);
	uint8_t slot = 0;
	bool bShoot = false;
};

inline sf::Packet &operator<<(sf::Packet &pkt, WorldUpdateData const &wud)
{
	pkt << wud.posDelta << wud.rot << wud.cannonRot << wud.slot << wud.bShoot;
	return pkt;
}

inline sf::Packet &operator>>(sf::Packet &pkt, WorldUpdateData &wud)
{
	sf::Vector2f posDelta;
	sf::Angle rot, cannonRot;
	uint8_t slot;
	bool bShoot;
	pkt >> posDelta >> rot >> cannonRot >> slot >> bShoot;
	wud = {posDelta, rot, cannonRot, slot, bShoot};
	return pkt;
}

class WorldClient
{
  public:
	WorldClient(sf::RenderWindow &window, EntityId ownPlayerId, int mapIndex,
	            std::array<PlayerState, MAX_PLAYERS> players, bool gameMusicEnabled = true);

	bool update(WorldUpdateData &wud);
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
	PowerupCooldownPanel m_powerupPanel;
	AmmunitionDisplay m_ammoDisplay;
	mutable MinimapClient m_minimap;
	sf::RenderWindow &m_window;
	MapClient m_mapClient;
	std::array<PlayerClient, MAX_PLAYERS> m_players;
	std::vector<ProjectileClient> m_projectiles;
	std::vector<ItemClient> m_items;
	EntityId const m_ownPlayerId;
	sf::View m_worldView;
	sf::View m_hudView;
	float m_zoomLevel = 1.0f;

  public:
	InterpClient m_interp;
};
