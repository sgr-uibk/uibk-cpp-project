#pragma once
#include "Map/MapState.h"
#include "Utilities.h"
#include <SFML/Graphics.hpp>
#include <SFML/System/Vector2.hpp>
#include <vector>

class MapClient
{
  public:
	explicit MapClient(MapState &state);

	void drawGroundTiles(sf::RenderWindow &window) const;
	void collectWallSprites(std::vector<RenderObject> &queue) const;

  private:
	static sf::Vector2i isoToScreen(sf::Vector2i tilePos, sf::Vector2i tileDim);
	template <typename SkipPolicy, typename Fn>
	void forEachTileInLayer(std::optional<RawLayer> const &layerOpt, Fn fn) const;

	MapState &m_state;
	sf::Texture const &m_tilesetTexture;
	sf::Sprite m_tilesetSprite;
};

struct SkipDestroyedWallsTag
{
};
struct IgnoreDestroyedWallsTag
{
};

template <typename SkipPolicy, typename Fn>
void MapClient::forEachTileInLayer(std::optional<RawLayer> const &layerOpt, Fn fn) const
{
	auto const tilesetOpt = m_state.getTileset();
	if(!layerOpt.has_value() || !tilesetOpt.has_value())
		return;

	auto const &layer = *layerOpt;
	auto const &tileset = *tilesetOpt;

	sf::Vector2i gridPos = {0, 0};
	for(; gridPos.y < layer.dim.y; ++gridPos.y)
	{
		for(gridPos.x = 0; gridPos.x < layer.dim.x; ++gridPos.x)
		{
			TileType const tileType = layer.data[gridPos.y * layer.dim.x + gridPos.x];
			if(tileType == AIR)
				continue;

			if constexpr(std::is_same_v<SkipPolicy, SkipDestroyedWallsTag>)
			{
				if(auto const *wall = m_state.getWallAtGridPos(gridPos); wall && wall->isDestroyed())
					continue;
			}

			sf::Vector2i const screenPos = isoToScreen(gridPos, tileset.mapTileDim);
			sf::Vector2i const spritesheetPos =
				sf::Vector2i{tileType % tileset.columns, tileType / tileset.columns}.componentWiseMul(tileset.tileDim);
			sf::IntRect const spriteRect{spritesheetPos, tileset.tileDim};
			sf::Vector2i const spritePos{screenPos.x - tileset.tileDim.x / 2,
			                             screenPos.y - tileset.tileDim.y + tileset.mapTileDim.y};
			fn(spriteRect, sf::Vector2f(spritePos));
		}
	}
}
