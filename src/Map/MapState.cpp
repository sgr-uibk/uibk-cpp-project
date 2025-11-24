#include "MapState.h"
#include "Networking.h"
#include <spdlog/spdlog.h>

MapState::MapState(sf::Vector2f size) : m_size(size)
{
	unsigned const width = size.x;
	unsigned const height = size.y;
	float wallThickness = 20.f;

	// Outer and inner walls
	addWall(0, 0, width, wallThickness);
	addWall(0, height - wallThickness, width, wallThickness);
	addWall(0, 0, wallThickness, height);
	addWall(width - wallThickness, 0, wallThickness, height);

	addWall(200, 100, wallThickness, 400); // vertical
	addWall(400, 200, 200, wallThickness); // horizontal
	addWall(600, 50, wallThickness, 300);  // vertical

	// predefined spawn points
	addSpawnPoint({100.f, 100.f}); // Top-left
	addSpawnPoint({700.f, 100.f}); // Top-right
	addSpawnPoint({100.f, 500.f}); // Bottom-left
	addSpawnPoint({700.f, 500.f}); // Bottom-right
}

void MapState::addWall(float x, float y, float w, float h, int health)
{
	m_walls.emplace_back(x, y, w, h, health);
}

void MapState::addSpawnPoint(sf::Vector2f spawn)
{
	m_spawns.push_back(spawn);
}

const std::vector<WallState> &MapState::getWalls() const
{
	return m_walls;
}

std::vector<WallState> &MapState::getWalls()
{
	return m_walls;
}

const std::vector<sf::Vector2f> &MapState::getSpawns() const
{
	return m_spawns;
}

bool MapState::isColliding(const sf::RectangleShape &r) const
{
	sf::FloatRect rbox = r.getGlobalBounds();
	for(auto const &w : m_walls)
	{
		if(w.isDestroyed())
			continue;

		sf::FloatRect wbox = w.getGlobalBounds();
		if(rbox.findIntersection(wbox).has_value())
		{
			return true;
		}
	}
	return false;
}
