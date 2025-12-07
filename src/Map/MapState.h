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

	void loadFromBlueprint(const MapBlueprint &bp);

	void addWall(float x, float y, float w, float h, int health = 100);
	void addSpawnPoint(sf::Vector2f spawn);
	void addItemSpawnZone(sf::Vector2f position, PowerupType itemType);
	void destroyWallAtGridPos(int x, int y); // Tile Swap: Clear wall tile when destroyed
	[[nodiscard]] bool isColliding(const sf::RectangleShape &r) const;
	[[nodiscard]] const std::vector<WallState> &getWalls() const;
	[[nodiscard]] std::vector<WallState> &getWalls();
	[[nodiscard]] const std::vector<sf::Vector2f> &getSpawns() const;
	[[nodiscard]] const std::vector<ItemSpawnZone> &getItemSpawnZones() const;
	[[nodiscard]] sf::Vector2f getSize() const;

	// Rendering Accessors (Consumed by MapClient)
	[[nodiscard]] const std::optional<RawTileset> &getTileset() const;
	[[nodiscard]] const std::vector<RawLayer> &getLayers() const;
	[[nodiscard]] std::optional<RawLayer> getGroundLayer() const;
	[[nodiscard]] std::optional<RawLayer> getWallsLayer() const;
	[[nodiscard]] const std::vector<WallState> &getWallStates() const;
	[[nodiscard]] const WallState *getWallAtGridPos(int x, int y) const;

	// Setters for preserving rendering data on client (when applying server snapshots)
	void setTileset(const std::optional<RawTileset> &tileset);
	void setGroundLayer(const std::optional<RawLayer> &layer);
	void setWallsLayer(const std::optional<RawLayer> &layer);

	static PowerupType stringToPowerupType(const std::string &str);

  private:
	sf::Vector2f m_size;
	std::vector<WallState> m_walls;
	std::vector<sf::Vector2f> m_spawns;
	std::vector<ItemSpawnZone> m_itemSpawnZones;

	std::optional<RawTileset> m_tileset;
	std::vector<RawLayer> m_layers;
};