#include <cmath>
#include <numeric>
#include <ranges>
#include <algorithm>
#include <spdlog/spdlog.h>
#include "WorldClient.h"
#include "Utilities.h"
#include "ResourceManager.h"
#include "../GameConfig.h"
#include "../UI/Scoreboard.h"

// https://www.trivialorwrong.com/2015/12/08/meta-sequences-in-c++.html
template <std::size_t N>
std::array<PlayerClient, N> make_players(std::array<PlayerState, N> &states, std::array<sf::Color, N> const &colors)
{
	return [&]<std::size_t... I>(std::index_sequence<I...>) {
		return std::array<PlayerClient, N>{PlayerClient(states[I], colors[I])...};
	}(std::make_index_sequence<N>{});
}

WorldClient::WorldClient(sf::RenderWindow &window, EntityId const ownPlayerId,
                         std::array<PlayerState, MAX_PLAYERS> &players)
	: m_bAcceptInput(true), m_pauseMenu(window), m_state(WorldState::fromTiledMap("map/arena.json")),
	  m_itemBar(m_state.getPlayerById(ownPlayerId), window), m_window(window), m_mapClient(m_state.getMap()),
	  m_players(make_players<MAX_PLAYERS>(players, PLAYER_COLORS)), m_ownPlayerId(ownPlayerId),
	  m_interp(m_state.getMap(), ownPlayerId, m_players)
{
	for(auto &ps : players)
	{
		if(ps.m_id == 0)
		{
			SPDLOG_LOGGER_WARN(spdlog::get("Client"), "Not setting state for invalid player 0!");
			continue;
		}

		m_state.setPlayer(ps);
		m_players[ps.m_id - 1].applyServerState(ps);
	}
	m_worldView = sf::View(sf::FloatRect({0, 0}, sf::Vector2f(window.getSize())));
	m_hudView = sf::View(m_worldView);
	m_frameClock.start();
	m_tickClock.start();

	// Initialize scoreboard
	m_scoreboard = std::make_unique<Scoreboard>(FontManager::inst().load("Font/LiberationSans-Regular.ttf"));
}

WorldClient::~WorldClient() = default;

void WorldClient::propagateUpdate(float dt)
{
	for(auto &pc : m_players)
		pc.update(dt);
	for(auto &proj : m_projectiles)
		proj.update(dt);
	for(auto &item : m_items)
		item.update(dt);
}

std::optional<sf::Packet> WorldClient::update(sf::Vector2f posDelta)
{
	int32_t const frameDelta = m_frameClock.restart().asSeconds();
	m_clientTick++;
	bool const bServerTickExpired = m_tickClock.getElapsedTime().asSeconds() >= UNRELIABLE_TICK_TIME;
	if(bServerTickExpired)
		m_tickClock.restart();
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
		sf::Packet pkt = createTickedPkt(UnreliablePktType::USE_ITEM, m_clientTick);
		pkt << m_ownPlayerId << m_itemBar.getSelection();
		return std::optional(pkt);
	}

	// TODO Can't shoot and move in the same frame like this
	bool const shoot =
		sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::Space) || sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);
	if(shoot && m_state.getPlayerById(m_ownPlayerId).canShoot())
	{
		sf::Packet pkt = createTickedPkt(UnreliablePktType::SHOOT, m_clientTick);
		pkt << m_ownPlayerId << aimAngle;
		return std::optional(pkt);
	}

	if(posDelta != sf::Vector2f{0, 0})
	{ // now we can safely normalize
		posDelta *= UNRELIABLE_TICK_HZ / (RENDER_TICK_HZ * posDelta.length());
		m_interp.predictMovement(posDelta);

		if(bServerTickExpired)
		{
			sf::Packet pkt = createTickedPkt(UnreliablePktType::MOVE, m_clientTick);
			pkt << m_ownPlayerId;
			pkt << m_interp.getCumulativeInputs(m_clientTick);
			pkt << aimAngle;
			return std::optional(pkt);
		}
	}

	return std::nullopt;
}

void WorldClient::draw(sf::RenderWindow &window) const
{
	// Update camera to follow player with isometric projection
	const PlayerState &ownPlayer = m_state.getPlayerById(m_ownPlayerId);
	sf::Vector2f playerCartCenter = ownPlayer.getPosition() + sf::Vector2f(PlayerState::logicalDimensions / 2.f);
	sf::Vector2f isoPlayerPos = cartesianToIso(playerCartCenter);

	sf::View cameraView = m_worldView;
	cameraView.setCenter(isoPlayerPos);
	cameraView.zoom(m_zoomLevel);
	window.setView(cameraView);

	window.clear(sf::Color::White);

	// Draw ground tiles first (no Y-sorting needed for ground)
	m_mapClient.drawGroundTiles(window);

	// Y-sorting: Collect all drawable objects with their Y coordinates
	std::vector<RenderObject> renderQueue;
	renderQueue.reserve(2000); // Pre-allocate for better performance

	// Add wall sprites from map
	m_mapClient.collectWallSprites(renderQueue);

	// Add items
	for(auto const &item : m_items)
	{
		if(item.getState().isActive())
		{
			sf::Vector2f itemPos = item.getShape().getPosition();
			RenderObject obj;
			obj.sortY = itemPos.y;
			obj.drawable = &item.getShape();
			renderQueue.push_back(obj);
		}
	}

	// Add projectiles
	for(auto const &proj : m_projectiles)
	{
		if(proj.getState().isActive())
		{
			sf::Vector2f projPos = proj.getShape().getPosition();
			RenderObject obj;
			obj.sortY = projPos.y;
			obj.drawable = &proj.getShape();
			renderQueue.push_back(obj);
		}
	}

	// Add players
	for(auto const &pc : m_players)
	{
		if(pc.getState().m_id != 0 && pc.getState().isAlive())
		{
			pc.collectRenderObjects(renderQueue);
		}
	}

	// Sort by Y coordinate (painter's algorithm)
	std::sort(renderQueue.begin(), renderQueue.end());

	// Draw all objects in sorted order
	for(auto const &obj : renderQueue)
		obj.draw(window);

	// HUD
	window.setView(m_hudView);
	m_itemBar.draw(window);
	m_pauseMenu.draw(window);

	// Draw scoreboard if active
	if(m_scoreboard && m_scoreboard->isShowing())
		m_scoreboard->draw(window);
}

// Applies server state that does not get interpolated, such as player inventory, items, etc.
void WorldClient::applyNonInterpState(WorldState const &snapshot)
{
	m_state = snapshot;

	auto const &projectileStates = m_state.getProjectiles();
	m_projectiles.clear();
	m_projectiles.reserve(projectileStates.size());
	for(auto const &projState : projectileStates)
	{
		if(projState.isActive())
			m_projectiles.emplace_back(projState);
	}

	auto const &itemStates = m_state.getItems();
	m_items.clear();
	m_items.reserve(itemStates.size());
	for(auto const &itemState : itemStates)
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
			{
				m_itemBar.handleKeyboardEvent(keyPress->scancode);

				// Handle zoom with +/- keys
				if(keyPress->scancode == sf::Keyboard::Scancode::Equal || keyPress->scancode == sf::Keyboard::Scancode::NumpadPlus)
					m_zoomLevel = std::max(0.5f, m_zoomLevel - 0.1f);
				else if(keyPress->scancode == sf::Keyboard::Scancode::Hyphen || keyPress->scancode == sf::Keyboard::Scancode::NumpadMinus)
					m_zoomLevel = std::min(2.0f, m_zoomLevel + 0.1f);
			}

		}
		else if(auto const *mouseEvent = event->getIf<sf::Event::MouseButtonPressed>())
		{
			if(m_pauseMenu.isPaused() && mouseEvent->button == sf::Mouse::Button::Left)
			{
				sf::Vector2f mousePos = m_window.mapPixelToCoords(mouseEvent->position, m_hudView);
				m_pauseMenu.handleClick(mousePos);
			}

			// Handle scoreboard clicks
			if(m_scoreboard && mouseEvent->button == sf::Mouse::Button::Left)
			{
				sf::Vector2f mousePos = m_window.mapPixelToCoords(mouseEvent->position, m_hudView);
				auto action = m_scoreboard->handleClick(mousePos);
				if(action == Scoreboard::ButtonAction::RETURN_TO_LOBBY)
					SPDLOG_LOGGER_INFO(spdlog::get("Client"), "Return to lobby requested from scoreboard");
			}
		}
		else if(auto const *scrollEvent = event->getIf<sf::Event::MouseWheelScrolled>())
		{
			if(m_window.hasFocus() && !m_pauseMenu.isPaused())
				m_itemBar.moveSelection(scrollEvent->delta);
		}
	}
}

void WorldClient::handleResize(sf::Vector2u newSize)
{
	m_worldView.setSize(sf::Vector2f(newSize));
	m_hudView.setSize(sf::Vector2f(newSize));
}

void WorldClient::showScoreboard(EntityId winnerId, const std::vector<PlayerStats> &playerStats)
{
	if(!m_scoreboard)
		return;

	// Convert to scoreboard format
	std::vector<Scoreboard::PlayerStats> scoreboardStats;
	for(const auto &stats : playerStats)
	{
		Scoreboard::PlayerStats sbStats;
		sbStats.id = stats.id;
		sbStats.name = stats.name;
		sbStats.kills = stats.kills;
		sbStats.deaths = stats.deaths;
		scoreboardStats.push_back(sbStats);
	}

	m_scoreboard->show(winnerId, scoreboardStats);
	SPDLOG_LOGGER_INFO(spdlog::get("Client"), "Scoreboard displayed with winner ID {}", winnerId);
}
