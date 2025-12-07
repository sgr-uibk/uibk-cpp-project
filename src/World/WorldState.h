#pragma once
#include <vector>
#include <string>
#include <SFML/Network.hpp>
#include "Map/MapState.h"
#include "Player/PlayerState.h"
#include "Projectile/ProjectileState.h"
#include "Item/ItemState.h"
#include "Networking.h"

class WorldState
{
  public:
	explicit WorldState(sf::Vector2f mapSize);
	static WorldState fromTiledMap(const std::string &tiledJsonPath);
	void update(float dt);
	void setPlayer(PlayerState const &p);

	std::array<PlayerState, MAX_PLAYERS> &getPlayers();
	PlayerState &getPlayerById(size_t id);
	const PlayerState &getPlayerById(size_t id) const;
	[[nodiscard]] MapState &getMap();
	[[nodiscard]] const MapState &getMap() const;

	uint32_t addProjectile(sf::Vector2f position, sf::Vector2f velocity, uint32_t ownerId, int damage = 25);
	void removeProjectile(uint32_t id);
	void removeInactiveProjectiles();
	std::vector<ProjectileState> &getProjectiles()
	{
		return m_projectiles;
	}
	const std::vector<ProjectileState> &getProjectiles() const
	{
		return m_projectiles;
	}

	uint32_t addItem(sf::Vector2f position, PowerupType type);
	void removeItem(uint32_t id);
	void removeInactiveItems();
	std::vector<ItemState> &getItems()
	{
		return m_items;
	}
	const std::vector<ItemState> &getItems() const
	{
		return m_items;
	}

	void checkProjectilePlayerCollisions();
	void checkProjectileWallCollisions();
	void checkPlayerItemCollisions();
	void checkPlayerPlayerCollisions();

	void clearWallDeltas();
	void markWallDestroyed(int gridX, int gridY);
	const std::vector<std::pair<int, int>> &getDestroyedWallDeltas() const;

	// serialization (full snapshot for players/items/projectiles, deltas for walls)
	void serialize(sf::Packet &pkt) const;
	void deserialize(sf::Packet &pkt);

	WorldState &operator=(const WorldState &);

  private:
	MapState m_map;
	// Players are not removed on disconnect,
	// as others can't join in the battle phase anyway.
	std::array<PlayerState, MAX_PLAYERS> m_players;
	std::vector<ProjectileState> m_projectiles;
	std::vector<ItemState> m_items;
	uint32_t m_nextProjectileId{1};
	uint32_t m_nextItemId{1};

	// Track walls destroyed this tick (grid coordinates)
	std::vector<std::pair<int, int>> m_destroyedWallsThisTick;
};
