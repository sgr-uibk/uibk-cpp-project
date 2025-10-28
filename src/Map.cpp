#include "Map.h"

bool Map::loadTextures(const std::string& folder)
{
    std::map<char, std::string> files = {
        {'#', "wall_brick.png"},
		{'g', "grass.png"},
        {'s', "stone.png"},
		{'w', "water.png"}
	    };

    for(auto& [c, file] : files)
    {
        sf::Texture tex;
        if(!tex.loadFromFile(folder + "/" + file))
        {
            std::cerr << "Failed to load texture: " << folder + "/" + file << std::endl;
            return false;
        }
        m_tileTextures[c] = tex;
    }

    return true;
}


bool Map::loadFromFile(const std::string& path, float tileSize)
{
    std::ifstream file(path);
    if(!file.is_open())
    {
        std::cerr << "Failed to open map file: " << path << std::endl;
        return false;
    }

    m_tiles.clear();

    std::string line;
    unsigned row = 0;
    while(std::getline(file, line))
    {
        for(unsigned col = 0; col < line.size(); ++col)
        {
            char c = line[col];
			if(c == 'P')
			{
				m_playerSpawn = sf::Vector2f(col * tileSize, row * tileSize);
				c = 'g';
			}

            auto it = m_tileTextures.find(c);
            if(it != m_tileTextures.end())
            {
				sf::Sprite sprite(it->second);
                float scaleX = tileSize / sprite.getTexture().getSize().x;
				float scaleY = tileSize / sprite.getTexture().getSize().y;
				sprite.setScale({scaleX, scaleY});
                sprite.setPosition(sf::Vector2f(col * tileSize, row * tileSize));
                m_tiles.push_back(sprite);

				if(c == '#' || c == 'w')
				{
					sf::RectangleShape wall(sf::Vector2f(tileSize, tileSize));
					wall.setPosition(sf::Vector2f(col * tileSize, row * tileSize));
					wall.setFillColor(sf::Color::Transparent);
					m_walls.push_back(wall);
				}
            }
        }
        ++row;
	}
	return true;
}


bool Map::isCollidingWithWalls(sf::FloatRect const &it) const
{
	bool collision = false;
	for(auto &wall : m_walls)
	{
		if(it.findIntersection(wall.getGlobalBounds()) != std::nullopt)
		{
			collision = true;
			break;
		}
	}
	return collision;
}

const std::vector<sf::RectangleShape> & Map::getWalls() const
{
	return m_walls;
}

void Map::draw(sf::RenderWindow& window) const
{
    for(const auto& sprite : m_tiles)
        window.draw(sprite);
}
