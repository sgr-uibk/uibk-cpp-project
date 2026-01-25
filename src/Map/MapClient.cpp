#include "MapClient.h"
#include "ResourceManager.h"
#include "Utilities.h"
#include <SFML/Graphics/Sprite.hpp>
#include <vector>

MapClient::MapClient(MapState &state)
	: m_state(state), m_tilesetTexture(TextureManager::inst().load("map/" + m_state.getTileset()->imagePath)),
	  m_tilesetSprite(sf::Sprite(m_tilesetTexture))
{
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
		m_state.getGroundLayer(), [&](sf::IntRect const &spriteRect, sf::Vector2f const screenPos) {
			sf::Sprite s = m_tilesetSprite;
			s.setTextureRect(spriteRect);
			s.setPosition(screenPos);
			window.draw(s);
		});
}

void MapClient::collectWallSprites(std::vector<RenderObject> &queue) const
{
	forEachTileInLayer<SkipDestroyedWallsTag>(m_state.getWallsLayer(), [&](sf::IntRect const &spriteRect, sf::Vector2f const screenPos) {
		sf::Sprite s = m_tilesetSprite;
		s.setTextureRect(spriteRect);
		s.setPosition(screenPos);
		RenderObject const obj{.sortY = screenPos.y + spriteRect.size.y, .tempSprite = std::move(s)};
		queue.push_back(obj);
	});
}
