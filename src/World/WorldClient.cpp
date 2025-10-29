#include "WorldClient.h"

#include <cmath>

WorldClient::WorldClient(sf::Vector2u dimensions, EntityId ownPlayerId, std::array<PlayerClient, MAX_PLAYERS> &players)
	: m_state(dimensions),
	  m_mapClient(m_state.map()),
	  m_players(players),
	  m_ownPlayerId(ownPlayerId)
{
	m_worldView = sf::View(sf::FloatRect({0, 0}, static_cast<sf::Vector2f>(dimensions)));
	m_hudView = sf::View(m_worldView);
}

sf::Packet WorldClient::update(float dt)
{
	bool const w = sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::W);
	bool const s = sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::S);
	bool const a = sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::A);
	bool const d = sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::D);

	sf::Packet pkt;
	const sf::Vector2f posDelta{static_cast<float>(d - a), static_cast<float>(s - w)};
	float const angRad = std::atan2(posDelta.x, -posDelta.y);
	sf::Angle const rot = sf::radians(angRad);

	// TODO: Disabled to explicitly show the server delay
	//m_players[m_ownPlayerId-1].applyLocalMove(m_state.map(), posDelta);
	//m_sprite.setRotation();

	if(posDelta != sf::Vector2f{0, 0} && m_tickClock.getElapsedTime() > sf::seconds(UNRELIABLE_TICK_RATE))
	{
		pkt << (uint8_t)UnreliablePktType::MOVE;
		pkt << posDelta;
		pkt << rot;
		m_tickClock.restart();
	}

	for(auto &pc : m_players)
		pc.update(dt);

	return pkt;

}

void WorldClient::draw(sf::RenderWindow &window) const
{
	window.setView(m_worldView);
	window.clear(sf::Color::White);
	m_mapClient.draw(window);
	for(auto const &pc : m_players)
		pc.draw(window);

	//window.setView(m_hudView);
	// todo draw HUD with m_hudView

}

void WorldClient::applyServerSnapshot(const WorldState &snapshot)
{
	m_state = snapshot;
	// the map is static for now (deserialize does not extract a Map) as serialize doesn't shove one in
	// m_mapClient.setMapState(m_state.map());

	auto const states = m_state.getPlayers();
	for(size_t i = 0; i < m_players.size(); ++i)
		m_players[i].applyServerState(states[i]);
}

WorldState &WorldClient::getState()
{
	return m_state;
}
