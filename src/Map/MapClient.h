#pragma once
#include "Map/MapState.h"
#include <SFML/Graphics.hpp>
#include <optional>

class MapClient
{
  public:
	explicit MapClient(MapState &state);

	void draw(sf::RenderWindow &window) const;

  private:
	void drawGroundTiles(sf::RenderWindow &window) const;
	void drawWallTiles(sf::RenderWindow &window) const;
	sf::Vector2f isoToScreen(int tileX, int tileY, int tileWidth, int tileHeight) const;

	MapState &m_state;
	std::optional<sf::Texture> m_tilesetTexture;
};
