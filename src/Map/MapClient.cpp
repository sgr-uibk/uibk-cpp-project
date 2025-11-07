#include "MapClient.h"

MapClient::MapClient(MapState &state, const std::string &mapFile, float tileSize) : m_state(state)
{
	std::ifstream file(mapFile);
	std::string line;
	unsigned row = 0;
	while(std::getline(file, line))
	{
		for(unsigned col = 0; col < line.size(); ++col)
		{
			char c = line[col];
			TileType type;
			switch(c)
			{
			case '#':
				type = TileType::Wall;
				break;
			case 'g':
				type = TileType::Grass;
				break;
			case 'w':
				type = TileType::Water;
				break;
			case 's':
				type = TileType::Stone;
				break;
			default:
				type = TileType::Grass;
				break;
			}
			m_tiles.emplace_back(type, sf::Vector2f(col * tileSize, row * tileSize), tileSize);
		}
		++row;
	}

	// Textures laden
	if(m_textures.find("wall") == m_textures.end())
		m_textures["wall"] = TextureManager::inst().load("tiles/wall_brick.png");
	if(m_textures.find("grass") == m_textures.end())
		m_textures["grass"] = TextureManager::inst().load("tiles/grass.png");
	if(m_textures.find("water") == m_textures.end())
		m_textures["water"] = TextureManager::inst().load("tiles/water.png");
	if(m_textures.find("stone") == m_textures.end())
		m_textures["stone"] = TextureManager::inst().load("tiles/stone.png");

	// Tiles setzen
	for(auto &tile : m_tiles)
	{
		switch(tile.getType())
		{
		case TileType::Wall:
			tile.setTexture(m_textures.at("wall"));
			break;
		case TileType::Grass:
			tile.setTexture(m_textures.at("grass"));
			break;
		case TileType::Water:
			tile.setTexture(m_textures.at("water"));
			break;
		case TileType::Stone:
			tile.setTexture(m_textures.at("stone"));
			break;
		}
	}
}

void MapClient::draw(sf::RenderWindow &window) const
{
	for(const auto &tile : m_tiles)
		tile.draw(window);
}