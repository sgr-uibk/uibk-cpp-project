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

sf::Vector2f MapClient::isoToScreen(int tileX, int tileY, int tileWidth, int tileHeight) const
{
	sf::Vector2f tile(tileX, tileY);
	float screenX = (tile.x - tile.y) * (tileWidth / 2.0f);
	float screenY = (tile.x + tile.y) * (tileHeight / 2.0f);
	return {screenX, screenY};
}

void MapClient::drawGroundTiles(sf::RenderWindow &window) const
{
	if(!m_tilesetTexture.has_value())
		return;

	auto const &groundLayer = m_state.getGroundLayer();
	auto const &tileset = m_state.getTileset();
	if(!groundLayer.has_value() || !tileset.has_value())
		return;

	int const mapWidth = groundLayer->width;
	int const mapHeight = groundLayer->height;
	int const tileSpriteW = tileset->tileWidth;
	int const tileSpriteH = tileset->tileHeight;
	int const mapTileW = tileset->mapTileWidth;
	int const mapTileH = tileset->mapTileHeight;
	int const columns = tileset->columns;
	int const firstGid = tileset->firstGid;

	for(int y = 0; y < mapHeight; ++y)
	{
		for(int x = 0; x < mapWidth; ++x)
		{
			int idx = y * mapWidth + x;
			int tileId = groundLayer->data[idx];
			if(tileId == 0)
				continue;

			int tileIndex = tileId - firstGid;
			// Safety check for tile index
			if(tileIndex < 0)
				continue;

			sf::Vector2i srcPos((tileIndex % columns) * tileSpriteW, (tileIndex / columns) * tileSpriteH);

			sf::Vector2f p = isoToScreen(x, y, mapTileW, mapTileH);

			sf::Vector2f spritePos = p - sf::Vector2f(tileSpriteW / 2.0f, tileSpriteH - (mapTileH / 2.0f));

			sf::Sprite tileSprite(*m_tilesetTexture);
			tileSprite.setTextureRect(sf::IntRect(srcPos, {tileSpriteW, tileSpriteH}));
			tileSprite.setPosition(spritePos);
			window.draw(tileSprite);
		}
	}
}

void MapClient::drawWallTiles(sf::RenderWindow &window) const
{
	if(!m_tilesetTexture.has_value())
		return;

	auto const &wallsLayer = m_state.getWallsLayer();
	auto const &tileset = m_state.getTileset();

	if(!wallsLayer.has_value() || !tileset.has_value())
		return;

	int const mapWidth = wallsLayer->width;
	int const mapHeight = wallsLayer->height;
	int const tileSpriteW = tileset->tileWidth;
	int const tileSpriteH = tileset->tileHeight;
	int const mapTileW = tileset->mapTileWidth;
	int const mapTileH = tileset->mapTileHeight;
	int const columns = tileset->columns;
	int const firstGid = tileset->firstGid;

	for(int y = 0; y < mapHeight; ++y)
	{
		for(int x = 0; x < mapWidth; ++x)
		{
			int idx = y * mapWidth + x;
			int tileId = wallsLayer->data[idx];

			// Skip empty tiles
			if(tileId == 0)
			{
				continue;
			}

			WallState const *wall = m_state.getWallAtGridPos(x, y);
			if(wall && wall->isDestroyed())
			{
				continue;
			}

			int tileIndex = tileId - firstGid;
			if(tileIndex < 0)
				continue;

			sf::Vector2i srcPos((tileIndex % columns) * tileSpriteW, (tileIndex / columns) * tileSpriteH);

			sf::Vector2f p = isoToScreen(x, y, mapTileW, mapTileH);
			sf::Vector2f spritePos = p - sf::Vector2f(tileSpriteW / 2.0f, tileSpriteH - mapTileH);

			sf::Sprite tileSprite(*m_tilesetTexture);
			tileSprite.setTextureRect(sf::IntRect(srcPos, {tileSpriteW, tileSpriteH}));
			tileSprite.setPosition(spritePos);
			window.draw(tileSprite);
		}
	}
}

void MapClient::collectWallSprites(std::vector<RenderObject> &queue) const
{
	if(!m_tilesetTexture.has_value())
		return;

	auto const &wallsLayer = m_state.getWallsLayer();
	auto const &tileset = m_state.getTileset();

	if(!wallsLayer.has_value() || !tileset.has_value())
		return;

	int const mapWidth = wallsLayer->width;
	int const mapHeight = wallsLayer->height;
	int const tileSpriteW = tileset->tileWidth;
	int const tileSpriteH = tileset->tileHeight;
	int const mapTileW = tileset->mapTileWidth;
	int const mapTileH = tileset->mapTileHeight;
	int const columns = tileset->columns;
	int const firstGid = tileset->firstGid;

	for(int y = 0; y < mapHeight; ++y)
	{
		for(int x = 0; x < mapWidth; ++x)
		{
			int idx = y * mapWidth + x;
			int tileId = wallsLayer->data[idx];

			if(tileId == 0)
				continue;

			WallState const *wall = m_state.getWallAtGridPos(x, y);
			if(wall && wall->isDestroyed())
				continue;

			int tileIndex = tileId - firstGid;
			if(tileIndex < 0)
				continue;

			sf::Vector2i srcPos((tileIndex % columns) * tileSpriteW, (tileIndex / columns) * tileSpriteH);

			sf::Vector2f p = isoToScreen(x, y, mapTileW, mapTileH);
			sf::Vector2f spritePos = p - sf::Vector2f(tileSpriteW / 2.0f, tileSpriteH - mapTileH);

			sf::Sprite tileSprite(*m_tilesetTexture);
			tileSprite.setTextureRect(sf::IntRect(srcPos, {tileSpriteW, tileSpriteH}));
			tileSprite.setPosition(spritePos);

			float depthY = spritePos.y + tileSpriteH;

			RenderObject obj;
			obj.sortY = depthY;
			obj.tempSprite = tileSprite;
			queue.push_back(obj);
		}
	}
}
