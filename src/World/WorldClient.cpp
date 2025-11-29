#include <cmath>
#include <spdlog/spdlog.h>
#include "WorldClient.h"
#include "Utilities.h"
#include "ResourceManager.h"

template <std::size_t N, std::size_t... I>
static std::array<PlayerClient, N> make_players_impl(std::array<PlayerState, N> &states,
                                                     const std::array<sf::Color, N> &colors, std::index_sequence<I...>)
{
	return {PlayerClient(states[I], colors[I])...};
}

template <std::size_t N>
static std::array<PlayerClient, N> make_players(std::array<PlayerState, N> &states,
                                                const std::array<sf::Color, N> &colors)
{
	return make_players_impl<N>(states, colors, std::make_index_sequence<N>{});
}

WorldClient::WorldClient(sf::RenderWindow &window, EntityId ownPlayerId, std::array<PlayerState, MAX_PLAYERS> &players)
	: m_bAcceptInput(true), m_pauseMenu(window), m_state(sf::Vector2f(window.getSize())),
	  m_itemBar(m_state.getPlayerById(ownPlayerId), window), m_window(window), m_mapClient(m_state.getMap()),
	  m_players(make_players<MAX_PLAYERS>(players, PLAYER_COLORS)), m_ownPlayerId(ownPlayerId)
{
	for(auto &ps : players)
	{
		if(ps.m_id == 0)
		{
			SPDLOG_WARN("Not setting state for invalid player 0!");
			continue;
		}

		m_state.setPlayer(ps);
		m_players[ps.m_id - 1].applyServerState(ps);
	}
	m_worldView = sf::View(sf::FloatRect({0, 0}, sf::Vector2f(window.getSize())));
	m_hudView = sf::View(m_worldView);
	m_frameClock.start();
	m_tickClock.start();
}

void WorldClient::propagateUpdate(float dt)
{
	for(auto &pc : m_players)
		pc.update(dt);
	for(auto &proj : m_projectiles)
		proj.update(dt);
	for(auto &item : m_items)
		item.update(dt);
}

std::optional<sf::Packet> WorldClient::update()
{
	float const frameDelta = m_frameClock.restart().asSeconds();

	propagateUpdate(frameDelta);
	if(m_pauseMenu.isPaused() || !m_bAcceptInput)
	{ // skip game logic
		return std::nullopt;
	}

	sf::Vector2i mousePixelPos = sf::Mouse::getPosition(m_window);
	sf::Vector2f mouseWorldPos = m_window.mapPixelToCoords(mousePixelPos, m_worldView);

	sf::Vector2f playerPos = m_state.getPlayerById(m_ownPlayerId).getPosition();
	sf::Vector2f aimDir = mouseWorldPos - playerPos;
	float aimRad = std::atan2(aimDir.x, -aimDir.y);
	sf::Angle aimAngle = sf::radians(aimRad);

	if(m_itemBar.handleItemUse())
	{
		sf::Packet pkt = createPkt(UnreliablePktType::USE_ITEM);
		pkt << m_ownPlayerId;
		pkt << m_itemBar.getSelection();
		return std::optional(pkt);
	}

	bool const w = sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::W);
	bool const s = sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::S);
	bool const a = sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::A);
	bool const d = sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::D);
	const sf::Vector2f posDelta{static_cast<float>(d - a), static_cast<float>(s - w)};

	// TODO Can't shoot and move in the same frame like this
	bool const shoot =
		sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::Space) || sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);
	if(shoot && m_state.getPlayerById(m_ownPlayerId).canShoot())
	{
		sf::Packet pkt = createPkt(UnreliablePktType::SHOOT);
		pkt << m_ownPlayerId;
		pkt << aimAngle;
		return std::optional(pkt);
	}

	if(m_tickClock.getElapsedTime() > sf::seconds(UNRELIABLE_TICK_RATE))
	{
		sf::Packet pkt = createPkt(UnreliablePktType::MOVE);
		pkt << m_ownPlayerId;
		pkt << posDelta;
		pkt << aimAngle;
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

	for(auto const &item : m_items)
		item.draw(window);

	for(auto const &proj : m_projectiles)
		proj.draw(window);

	for(auto const &pc : m_players)
	{
		if(pc.getState().m_id != 0)
			pc.draw(window);
	}

	// HUD
	window.setView(m_hudView);
	m_itemBar.draw(window);
	m_pauseMenu.draw(window);
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

	const auto &projectileStates = m_state.getProjectiles();
	m_projectiles.clear();
	m_projectiles.reserve(projectileStates.size());
	for(const auto &projState : projectileStates)
	{
		if(projState.isActive())
			m_projectiles.emplace_back(projState);
	}

	const auto &itemStates = m_state.getItems();
	m_items.clear();
	m_items.reserve(itemStates.size());
	for(const auto &itemState : itemStates)
	{
		if(itemState.isActive())
			m_items.emplace_back(itemState);
	}
}

WorldState &WorldClient::getState()
{
	return m_state;
}

// TODO Move/keep logic out of this function, it's just a dispatcher.
void WorldClient::pollEvents()
{
	while(std::optional const event = m_window.pollEvent())
	{
		if(event->is<sf::Event::Closed>())
			m_window.close();
		else if(event->is<sf::Event::FocusLost>())
			m_bAcceptInput = false;
		else if(event->is<sf::Event::FocusGained>())
			m_bAcceptInput = true;
		else if(auto const *keyPress = event->getIf<sf::Event::KeyPressed>())
		{
			m_pauseMenu.handleKeyboardEvent(*keyPress);

			if(m_bAcceptInput && !m_pauseMenu.isPaused())
				m_itemBar.handleKeyboardEvent(keyPress->scancode);
		}
		else if(auto const *mouseEvent = event->getIf<sf::Event::MouseButtonPressed>())
		{
			if(m_pauseMenu.isPaused() && mouseEvent->button == sf::Mouse::Button::Left)
			{
				sf::Vector2f mousePos = m_window.mapPixelToCoords(mouseEvent->position, m_hudView);
				m_pauseMenu.handleClick(mousePos);
			}
		}

		else if(auto const *scrollEvent = event->getIf<sf::Event::MouseWheelScrolled>())
		{
			if(m_window.hasFocus() && !m_pauseMenu.isPaused())
				m_itemBar.moveSelection(scrollEvent->delta);
		}
	}
}
