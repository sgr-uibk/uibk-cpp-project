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
	template <typename SkipPolicy, typename Fn>
	void forEachTileInLayer(std::optional<RawLayer> const &layerOpt, Fn fn) const;

	MapState &m_state;
	std::optional<sf::Texture> m_tilesetTexture;
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
	if(!m_tilesetTexture.has_value() || !layerOpt.has_value() || !tilesetOpt.has_value())
		return;

	auto const &layer = *layerOpt;
	auto const &tileset = *tilesetOpt;

	sf::Vector2i gridPos = {0, 0};
	for(; gridPos.y < layer.dim.y; ++gridPos.y)
	{
		for(gridPos.x = 0; gridPos.x < layer.dim.x; ++gridPos.x)
		{
			int const tileId = layer.data[gridPos.y * layer.dim.x + gridPos.x];
			int const tileIndex = tileId - tileset.firstGid;
			if(tileId == 0 || tileIndex < 0)
				continue;

			if constexpr(std::is_same_v<SkipPolicy, SkipDestroyedWallsTag>)
			{
				if(auto const *wall = m_state.getWallAtGridPos(gridPos); wall && wall->isDestroyed())
					continue;
			}

			sf::Vector2i const srcCell{tileIndex % tileset.columns, tileIndex / tileset.columns};
			auto const screenPos = isoToScreen(gridPos, tileset.mapTileDim);

			fn(tileset.tileDim, tileset.mapTileDim, srcCell, screenPos);
		}
	}
}
