#include "WorldClient.h"
#include "Utilities.h"

// https://www.trivialorwrong.com/2015/12/08/meta-sequences-in-c++.html
template <std::size_t N>
std::array<PlayerClient, N> make_players(std::array<PlayerState, N> &states, std::array<sf::Color, N> const &colors)
{
	return [&]<std::size_t... I>(std::index_sequence<I...>) {
		return std::array<PlayerClient, N>{PlayerClient(states[I], colors[I])...};
	}(std::make_index_sequence<N>{});
}

WorldClient::WorldClient(sf::RenderWindow &window, EntityId ownPlayerId, std::array<PlayerState, MAX_PLAYERS> &players)
	: m_bAcceptInput(true), m_window(window), m_state(sf::Vector2f(window.getSize())), m_mapClient(m_state.getMap()),
	  // Members must be initialized before the constructor body runs,
      // so this initializer needs to convert the PlayerStates to PlayerClients *at compile time*
	  m_players(make_players<MAX_PLAYERS>(players, PLAYER_COLORS)), m_ownPlayerId(ownPlayerId)
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

std::optional<sf::Packet> WorldClient::update(sf::Vector2f const posDelta)
{
	float const frameDelta = m_frameClock.restart().asSeconds();

	for(auto &pc : m_players)
		pc.update(frameDelta);

	// TODO: Disabled to explicitly show the server delay
	// m_players[m_ownPlayerId-1].applyLocalMove(m_state.map(), posDelta);
	// m_sprite.setRotation();

	if(posDelta != sf::Vector2f{0, 0} && m_bAcceptInput &&
	   m_tickClock.getElapsedTime() > sf::seconds(UNRELIABLE_TICK_TIME))
	{
		sf::Packet pkt = createPkt(UnreliablePktType::MOVE);
		pkt << m_ownPlayerId;
		pkt << posDelta;
		m_tickClock.restart();
		return std::optional(pkt);
	}

	return std::nullopt;
}

void WorldClient::draw(sf::RenderWindow &window) const
{
	window.setView(m_worldView);
	window.clear(sf::Color::White);
	m_mapClient.draw(window);
	for(auto const &pc : m_players)
		pc.draw(window);

	// window.setView(m_hudView);
	//  todo draw HUD with m_hudView
}

void WorldClient::applyServerSnapshot(WorldState const &snapshot)
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

void WorldClient::pollEvents()
{
	while(std::optional const event = m_window.pollEvent())
	{
		if(event->is<sf::Event::Closed>())
		{
			m_window.close();
		}
		else if(auto const *keyPressed = event->getIf<sf::Event::KeyPressed>())
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
