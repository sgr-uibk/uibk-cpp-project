#include "MapState.h"
#include "Networking.h"

MapState::MapState(sf::Vector2f size) : m_size(size) {}

void MapState::addWall(float x, float y, float w, float h)
{
	sf::RectangleShape r({w, h});
	r.setPosition({x, y});
	r.setFillColor(sf::Color::Black);
	m_walls.push_back(std::move(r));
}

const std::vector<sf::RectangleShape> &MapState::getWalls() const
{
	return m_walls;
}

bool MapState::isColliding(const sf::RectangleShape &r) const
{
	sf::FloatRect rbox = r.getGlobalBounds();
	for(auto const &w : m_walls)
	{
		if(rbox.findIntersection(w.getGlobalBounds()))
			return true;
	}
	return false;
}

void MapState::reset(sf::Vector2f size)
{
    m_size = size;
    m_tiles.clear();
	m_playerSpawns.clear();
}

sf::Vector2f MapState::getPlayerSpawn(int playerId) const {
	auto it = m_playerSpawns.find(playerId);
    if(it != m_playerSpawns.end()) return it->second;
        return {0.f,0.f}; 
}

void MapState::addTile(TileType type, sf::Vector2f pos, float size)
{
    m_tiles.emplace_back(type, pos, size);
}

void MapState::setPlayerSpawn(int playerId, sf::Vector2f pos) { 
	m_playerSpawns[playerId] = pos; 
}

std::vector<Tile> &MapState::getTiles() {
	return m_tiles;
}

sf::Vector2f MapState::getSize() const {
		return m_size;
}
