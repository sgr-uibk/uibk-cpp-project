#include "WorldClient.h"
#include "Utilities.h"

#include <numeric>
#include <ranges>

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

std::optional<sf::Packet> WorldClient::update(sf::Vector2f posDelta)
{
	float const frameDelta = m_frameClock.restart().asSeconds();
	m_clientTick++;

	for(auto &pc : m_players)
		pc.update(frameDelta);
	if(m_bAcceptInput && posDelta != sf::Vector2f{0, 0})
	{ // now we can safely normalize
		posDelta *= UNRELIABLE_TICK_HZ / (RENDER_TICK_HZ * posDelta.length());

		m_inputAcc += posDelta;
		m_players[m_ownPlayerId - 1].applyLocalMove(m_state.m_map, posDelta); // predict

		if(m_tickClock.getElapsedTime().asMilliseconds() >= UNRELIABLE_TICK_MS)
		{
			m_inputInflightBuffer.emplace_back(m_clientTick, m_inputAcc);
			sf::Packet pkt = createTickedPkt(UnreliablePktType::MOVE, m_clientTick);
			pkt << m_ownPlayerId;
			pkt << m_inputAcc;
			m_inputAcc = {0, 0}; // now in flight
			m_tickClock.restart();
			return std::make_optional(pkt);
		}
	}

	for(auto &pc : m_players)
		pc.update();

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

void WorldClient::reconcileLocalPlayer(Tick ackedTick, WorldState const &snap)
{
	size_t const id = m_ownPlayerId - 1;
	PlayerState authState = snap.m_players[id];
	PlayerState localState = m_players[id].getState();

	// Erase inputs that made it into the received snapshot
	while(!m_inputInflightBuffer.empty() && m_inputInflightBuffer.front().first <= ackedTick)
		m_inputInflightBuffer.pop_front();

	auto err = authState.m_pos - localState.m_pos;
	if(err.length() == 0)
	{ // Prediction hit! We're done here.
		return;
	}

	// Server disagrees, have to accept authoritative state, replay local inputs
	m_players[id].applyServerState(authState);

	for(auto const &delta : m_inputInflightBuffer | std::views::values)
		m_players[id].applyLocalMove(m_state.m_map, delta);  // replay in flight
	m_players[id].applyLocalMove(m_state.m_map, m_inputAcc); // replay pre-in flight inputs
	auto const replayedState = m_players[id].getState();
	err = replayedState.m_pos - localState.m_pos;
	if(err.length() > 1e-3)
		SPDLOG_WARN("Reconciliation error ({},{})", err.x, err.y);
}

void WorldClient::interpolateEnemies() const
{
	// Render‑time interpolation
	long const renderTick = m_authTick - 1;
	if(renderTick < 0)
	{
		SPDLOG_INFO("Interpolation not yet ready");
		return;
	}

	struct interpState
	{
		size_t step = size_t(-1);
		Ticked<WorldState> const *node0 = nullptr;
		Ticked<WorldState> const *node1 = nullptr;
	};
	static interpState s;

	if(s.step >= RENDER_TICK_HZ / UNRELIABLE_TICK_HZ)
	{ // completed interpolation, get the next pair
		s.step = 0;
		s.node0 = &m_snapshotBuffer.get(1);
		s.node1 = &m_snapshotBuffer.get(0);
	}

	auto const [t0, s0] = *s.node0;
	auto const [t1, s1] = *s.node1;

	if(t0 + 1 != t1 && t0 && t1)
	{
		SPDLOG_ERROR("SS buffer out of sequence: {},{}", t0, t1);
		return;
	}

	float const alpha = s.step++ * UNRELIABLE_TICK_HZ / float(RENDER_TICK_HZ);

	for(size_t i = 0; i < MAX_PLAYERS; ++i)
	{
		auto &p0 = s0.m_players[i];
		auto &p1 = s1.m_players[i];
		size_t id = i + 1;
		if(id == m_ownPlayerId)
			continue; // own player is predicted, not interpolated

		m_players[i].interp(p0, p1, alpha);
	}
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

void WorldClient::reset()
{
	m_authTick = 0;
	m_clientTick = 0;
	m_snapshotBuffer.clear();
}
