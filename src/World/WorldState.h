#pragma once
#include <vector>
#include <SFML/Network.hpp>
#include "Map/MapState.h"
#include "Player/PlayerState.h"
#include "Projectile/ProjectileState.h"
#include "Item/ItemState.h"
#include "Networking.h"

struct WorldState
{
	explicit WorldState(sf::Packet &pkt);                                               // Client: from network packet
	WorldState(int mapIndex, std::array<PlayerState, MAX_PLAYERS> playersInit);         // Server/Client: from map index
	WorldState(sf::Vector2f mapSize, std::array<PlayerState, MAX_PLAYERS> playersInit); // Legacy: basic map
	void update(float dt);
	void setPlayer(PlayerState const &p);

	std::array<PlayerState, MAX_PLAYERS> &getPlayers();
	PlayerState &getPlayerById(size_t id);
	PlayerState const &getPlayerById(size_t id) const;
	[[nodiscard]] MapState &getMap();
	[[nodiscard]] MapState const &getMap() const;

	uint32_t addProjectile(sf::Vector2f position, sf::Vector2f velocity, uint32_t ownerId, int damage = 25);
	void removeProjectile(uint32_t id);
	void removeInactiveProjectiles();
	uint32_t addItem(sf::Vector2f position, PowerupType type);
	void removeItem(uint32_t id);
	void removeInactiveItems();
	std::vector<ProjectileState> &getProjectiles()
	{
		return m_projectiles;
	}
	std::vector<ProjectileState> const &getProjectiles() const
	{
		return m_projectiles;
	}
	std::vector<ItemState> &getItems()
	{
		return m_items;
	}
	std::vector<ItemState> const &getItems() const
	{
		return m_items;
	}

	void checkProjectilePlayerCollisions();
	void checkProjectileWallCollisions();
	void checkPlayerItemCollisions();
	void checkPlayerPlayerCollisions();

	void clearWallDeltas();
	void markWallDestroyed(sf::Vector2i gridPos);
	[[nodiscard]] std::vector<sf::Vector2i> const &getDestroyedWallDeltas() const;

	// serialization (full snapshot for players/items/projectiles, deltas for walls)
	void serialize(sf::Packet &pkt) const;

	WorldState &operator=(WorldState const &) = delete;
	WorldState &operator=(WorldState &) = delete;
	WorldState &operator=(WorldState) = delete;
	void assignExcludingInterp(WorldState const &);

	MapState m_map;
	std::array<PlayerState, MAX_PLAYERS> m_players;
	std::vector<ProjectileState> m_projectiles;
	std::vector<ItemState> m_items;
	uint32_t m_nextProjectileId{1};
	uint32_t m_nextItemId{1};
	std::vector<sf::Vector2i> m_destroyedWallsThisTick;

  private:
	void deserialize(sf::Packet &pkt);
};
