#include "WorldClient.h"

#include <cmath>
#include <spdlog/spdlog.h>

template <std::size_t N, std::size_t... I>
static std::array<PlayerClient, N> make_players_impl(
	std::array<PlayerState, N> &states,
	const std::array<sf::Color, N> &colors,
	std::index_sequence<I...>)
{
	return {PlayerClient(states[I], colors[I])...};
}

template <std::size_t N>
static std::array<PlayerClient, N> make_players(
	std::array<PlayerState, N> &states,
	const std::array<sf::Color, N> &colors)
{
	return make_players_impl<N>(states, colors, std::make_index_sequence<N>{});
}

WorldClient::WorldClient(sf::RenderWindow &window, EntityId ownPlayerId,
                         std::array<PlayerState, MAX_PLAYERS> &players)
	: m_bAcceptInput(true),
	  m_window(window),
	  m_state(sf::Vector2f(window.getSize())),
	  m_mapClient(m_state.getMap()),
	  // Members must be initialized before the constructor body runs,
	  // so this initializer needs to convert the PlayerStates to PlayerClients *at compile time*
	  m_players(make_players<MAX_PLAYERS>(players, PLAYER_COLORS)),
	  m_ownPlayerId(ownPlayerId)
{
	for(auto &ps : players)
	{
		m_state.setPlayer(ps);
		m_players[ps.m_id - 1].applyServerState(ps);
	}
	m_worldView = sf::View(sf::FloatRect({0, 0}, sf::Vector2f(window.getSize())));
	m_hudView = sf::View(m_worldView);
	m_frameClock.start();
	m_tickClock.start();
}

sf::Packet WorldClient::update()
{
	float const frameDelta = m_frameClock.restart().asSeconds();
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

	if(posDelta != sf::Vector2f{0, 0} && m_bAcceptInput
	   && m_tickClock.getElapsedTime() > sf::seconds(UNRELIABLE_TICK_RATE))
	{
		pkt << uint8_t(UnreliablePktType::MOVE);
		pkt << m_ownPlayerId;
		pkt << posDelta;
		pkt << rot;
		m_tickClock.restart();
	}

	for(auto &pc : m_players)
		pc.update(frameDelta);

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
	// the map is static for now (deserialize does not extract a Map) as serialize doesn't shove one in.
	// m_mapClient.setMapState(m_state.map());

	auto const states = m_state.getPlayers();
	assert(states.size() == m_players.size());
	for(size_t i = 0; i < m_players.size(); ++i)
		m_players[i].applyServerState(states[i]);
}

WorldState &WorldClient::getState()
{
	return m_state;
}

void WorldClient::pollInputs()
{
	while(const std::optional event = m_window.pollEvent())
	{
		if(event->is<sf::Event::Closed>())
		{
			m_window.close();
		}
		else if(const auto *keyPressed = event->getIf<sf::Event::KeyPressed>())
		{
			if(keyPressed->scancode == sf::Keyboard::Scancode::Escape)
				m_window.close();
		}
		else if(event->is<sf::Event::FocusLost>())
			m_bAcceptInput = false;
		else if(event->is<sf::Event::FocusGained>())
			m_bAcceptInput = true;
	}
}
