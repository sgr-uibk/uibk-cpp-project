#pragma once
#include <vector>
#include <string>
#include <optional>
#include <SFML/Graphics.hpp>
#include "WallState.h"
#include "Powerup.h"
#include "MapData.h"

struct ItemSpawnZone
{
	sf::Vector2f position;
	PowerupType itemType;
};

class MapState
{
  public:
	explicit MapState(sf::Vector2f size = {0.f, 0.f});

	void loadFromBlueprint(MapBlueprint const &bp);

	void addSpawnPoint(sf::Vector2f spawn);
	void addItemSpawnZone(sf::Vector2f position, PowerupType itemType);
	static bool isRendered(WallState const &wall, RawLayer const &layer);
	void destroyWallAtGridPos(sf::Vector2i pos); // Tile Swap: Clear wall tile when destroyed
	[[nodiscard]] bool isColliding(sf::RectangleShape const &r) const;
	[[nodiscard]] std::vector<WallState> const &getWalls() const;
	[[nodiscard]] std::vector<WallState> &getWalls();
	[[nodiscard]] std::vector<sf::Vector2f> const &getSpawns() const;
	[[nodiscard]] std::vector<ItemSpawnZone> const &getItemSpawnZones() const;
	[[nodiscard]] sf::Vector2f getSize() const;

	// Rendering Accessors (Consumed by MapClient)
	[[nodiscard]] std::optional<RawTileset> const &getTileset() const;
	[[nodiscard]] std::vector<RawLayer> const &getLayers() const;
	[[nodiscard]] std::optional<RawLayer> getGroundLayer() const;
	[[nodiscard]] std::optional<RawLayer> getWallsLayer() const;
	[[nodiscard]] WallState const *getWallAtGridPos(sf::Vector2i pos) const;

	// Setters for preserving rendering data on client (when applying server snapshots)
	void setTileset(std::optional<RawTileset> const &tileset);
	void setGroundLayer(std::optional<RawLayer> const &layer);
	void setWallsLayer(std::optional<RawLayer> const &layer);

	static PowerupType stringToPowerupType(std::string const &str);

  private:
	// Finds a layer by name, returning a pointer (null if not found).
	[[nodiscard]] RawLayer *findLayerByName(std::string const &name);
	[[nodiscard]] RawLayer const *findLayerByName(std::string const &name) const;

	static constexpr int BASE_WALL_HEALTH = 100;
	static constexpr int REINFORCED_HEALTH_MULTIPLIER = 50;

	sf::Vector2f m_size;
	std::vector<WallState> m_walls;
	std::vector<sf::Vector2f> m_spawns;
	std::vector<ItemSpawnZone> m_itemSpawnZones;

	std::optional<RawTileset> m_tileset;
	std::vector<RawLayer> m_layers;
};
