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
	template <bool bSkipDestroyedWalls, typename Fn>
	void forEachTileInLayer(std::optional<RawLayer> const &layerOpt, Fn fn) const;

	MapState &m_state;
	std::optional<sf::Texture> m_tilesetTexture;
};

template <bool bSkipDestroyedWalls, typename Fn>
void MapClient::forEachTileInLayer(std::optional<RawLayer> const &layerOpt, Fn fn) const
{
	auto const tilesetOpt = m_state.getTileset();
	if(!m_tilesetTexture.has_value() || !layerOpt.has_value() || !tilesetOpt.has_value())
		return;

	auto const &layer = *layerOpt;
	auto const &tileset = *tilesetOpt;

	for(int y = 0; y < layer.dim.y; ++y)
	{
		for(int x = 0; x < layer.dim.x; ++x)
		{
			int const tileId = layer.data[y * layer.dim.x + x];
			int const tileIndex = tileId - tileset.firstGid;
			if(tileId == 0 || tileIndex < 0)
				continue;

			sf::Vector2i const gridPos{x, y};
			if(auto const *wall = m_state.getWallAtGridPos(gridPos); bSkipDestroyedWalls && wall && wall->isDestroyed())
				continue;

			sf::Vector2i const srcCell{tileIndex % tileset.columns, tileIndex / tileset.columns};
			auto const screenPos = isoToScreen(gridPos, tileset.mapTileDim);

			fn(tileset.tileDim, tileset.mapTileDim, srcCell, screenPos);
		}
	}
}
