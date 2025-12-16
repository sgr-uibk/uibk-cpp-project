#pragma once
#include "Map/MapState.h"
#include "Utilities.h"
#include <SFML/Graphics.hpp>
#include <optional>
#include <vector>

class MapClient
{
  public:
	explicit MapClient(MapState &state);

	void drawGroundTiles(sf::RenderWindow &window) const;
	void collectWallSprites(std::vector<RenderObject> &queue) const;

  private:
	void drawWallTiles(sf::RenderWindow &window) const;
	static sf::Vector2i isoToScreen(sf::Vector2i tilePos, sf::Vector2i tileDim);

	MapState &m_state;
	std::optional<sf::Texture> m_tilesetTexture;
};
