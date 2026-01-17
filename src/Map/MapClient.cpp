#include "MapClient.h"
#include "ResourceManager.h"
#include "Utilities.h"
#include <vector>
#include <spdlog/spdlog.h>
#include <filesystem>

MapClient::MapClient(MapState &state) : m_state(state)
{
	if(auto const &tileset = m_state.getTileset())
	{
		try
		{
			std::string tilesetRelPath = "map/" + tileset->imagePath;
			auto fullPath = g_assetPathResolver.resolveRelative(tilesetRelPath);

			sf::Texture texture;
			if(texture.loadFromFile(fullPath.string()))
			{
				m_tilesetTexture = std::move(texture);
				spdlog::info("Loaded tileset texture: {}", fullPath.string());
			}
			else
			{
				spdlog::error("Failed to load tileset texture: {}", fullPath.string());
			}
		}
		catch(std::exception const &e)
		{
			spdlog::error("Failed to resolve tileset path: {} - {}", tileset->imagePath, e.what());
		}
	}
}

sf::Vector2i MapClient::isoToScreen(sf::Vector2i tilePos, sf::Vector2i tileDim)
{
	tilePos = {tilePos.x - tilePos.y, tilePos.x + tilePos.y};
	tilePos = tilePos.componentWiseMul(tileDim / 2);
	assert(tileDim.x > 2 && tileDim.y > 2); // ensure we're not loosing precision here
	return tilePos;
}

void MapClient::drawGroundTiles(sf::RenderWindow &window) const
{
	forEachTileInLayer<IgnoreDestroyedWallsTag>(
		m_state.getGroundLayer(),
		[&](sf::Vector2i tileSprite, sf::Vector2i mapTileDim, sf::Vector2i srcPixel, sf::Vector2i screenPos) {
			sf::Vector2f const spritePos{screenPos.x - tileSprite.x / 2.f,
		                                 float(screenPos.y) - tileSprite.y + 2 * mapTileDim.y};
			sf::Sprite s(*m_tilesetTexture, {srcPixel, tileSprite});
			s.setPosition(spritePos);
			window.draw(s);
		});
}

void MapClient::drawWallTiles(sf::RenderWindow &window) const
{
	forEachTileInLayer<SkipDestroyedWallsTag>(
		m_state.getWallsLayer(),
		[&](sf::Vector2i tileSprite, sf::Vector2i mapTileDim, sf::Vector2i srcPixel, sf::Vector2i screenPos) {
			sf::Vector2f const spritePos{screenPos.x - tileSprite.x / 2.f,
		                                 float(screenPos.y) - tileSprite.y + mapTileDim.y};
			sf::Sprite s(*m_tilesetTexture, {srcPixel, tileSprite});
			s.setPosition(spritePos);
			window.draw(s);
		});
}

void MapClient::collectWallSprites(std::vector<RenderObject> &queue) const
{
	forEachTileInLayer<SkipDestroyedWallsTag>(
		m_state.getWallsLayer(),
		[&](sf::Vector2i tileSprite, sf::Vector2i mapTileDim, sf::Vector2i srcPixel, sf::Vector2i screenPos) {
			sf::Vector2f const spritePos{screenPos.x - tileSprite.x / 2.f,
		                                 float(screenPos.y) - tileSprite.y + mapTileDim.y};
			sf::Sprite s(*m_tilesetTexture, {srcPixel, tileSprite});
			s.setPosition(spritePos);
			RenderObject const obj{.sortY = spritePos.y + tileSprite.y, .tempSprite = std::move(s)};
			queue.push_back(obj);
		});
}
