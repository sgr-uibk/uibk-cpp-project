#include <cmath>
#include <spdlog/spdlog.h>
#include "WorldClient.h"
#include "Utilities.h"
#include "ResourceManager.h"

// https://www.trivialorwrong.com/2015/12/08/meta-sequences-in-c++.html
template <std::size_t N>
std::array<PlayerClient, N> make_players(std::array<PlayerState, N> &states, std::array<sf::Color, N> const &colors)
{
	return [&]<std::size_t... I>(std::index_sequence<I...>) {
		return std::array<PlayerClient, N>{PlayerClient(states[I], colors[I])...};
	}(std::make_index_sequence<N>{});
}

WorldClient::WorldClient(sf::RenderWindow &window, EntityId const ownPlayerId, int mapIndex,
                         std::array<PlayerState, MAX_PLAYERS> players)
	: m_bAcceptInput(true), m_pauseMenu(window),
	  m_state(mapIndex, std::move(players)),
	  m_itemBar(m_state.getPlayerById(ownPlayerId), window),
	  m_healthBar(sf::Vector2f(20.f, 20.f), sf::Vector2f(220.f, 28.f), m_state.getPlayerById(ownPlayerId).getMaxHealth()),
	  m_powerupPanel(m_state.getPlayerById(ownPlayerId), window),
	  m_ammoDisplay(m_state.getPlayerById(ownPlayerId), window),
	  m_window(window), m_mapClient(m_state.getMap()),
	  m_players(make_players<MAX_PLAYERS>(m_state.m_players, PLAYER_COLORS)),
	  m_ownPlayerId(ownPlayerId),
	  m_interp(m_state.getMap(), ownPlayerId, m_players)
{
	m_worldView = sf::View(sf::FloatRect({0, 0}, sf::Vector2f(window.getSize())));
	m_hudView = sf::View(m_worldView);
	m_frameClock.start();
	m_tickClock.start();
}

void WorldClient::propagateUpdate(float const dt)
{
	for(auto &pc : m_players)
		pc.update(dt);
	for(auto &proj : m_projectiles)
		proj.update(dt);
	for(auto &item : m_items)
		item.update(dt);
	m_powerupPanel.update(dt);
	m_ammoDisplay.update(dt);
}

std::optional<sf::Packet> WorldClient::update(sf::Vector2f posDelta)
{
	float const frameDelta = m_frameClock.restart().asSeconds();
	m_clientTick++;
	bool const bServerTickExpired = m_tickClock.getElapsedTime().asSeconds() >= UNRELIABLE_TICK_TIME;
	if(bServerTickExpired)
		m_tickClock.restart();
	propagateUpdate(frameDelta);

	PlayerState const &ownPlayer = m_state.getPlayerById(m_ownPlayerId);
	m_healthBar.setHealth(ownPlayer.getHealth());
	m_healthBar.setMaxHealth(ownPlayer.getMaxHealth());

	if(m_pauseMenu.isPaused() || !m_bAcceptInput || !m_window.hasFocus())
		return std::nullopt;

	sf::Vector2f const playerCenter = ownPlayer.getPosition() + PlayerState::logicalDimensions / 2.f;

	sf::View cameraView = m_worldView;
	cameraView.setCenter(cartesianToIso(playerCenter));
	cameraView.zoom(m_zoomLevel);

	sf::Vector2i const mousePixelPos = sf::Mouse::getPosition(m_window);
	sf::Vector2f const mouseCartPos = isoToCartesian(m_window.mapPixelToCoords(mousePixelPos, cameraView));
	sf::Vector2f const dir = mouseCartPos - playerCenter;
	sf::Angle const aimAngle = (sf::radians(std::atan2(dir.y, dir.x)) + sf::degrees(90.f)).wrapUnsigned();

	if(m_ownPlayerId > 0 && m_ownPlayerId <= MAX_PLAYERS)
		m_players[m_ownPlayerId - 1].setTurretRotation(aimAngle);

	if(m_itemBar.handleItemUse())
	{
		size_t const slot = m_itemBar.getSelection();
		PowerupType const itemType = ownPlayer.getInventoryItem(static_cast<int>(slot));

		if(itemType != PowerupType::NONE && !ownPlayer.canUsePowerup(itemType))
			m_powerupPanel.triggerCooldownFlash(itemType);
		else
		{
			sf::Packet pkt = createTickedPkt(UnreliablePktType::USE_ITEM, m_clientTick);
			pkt << slot;
			return pkt;
		}
	}

	bool const wantsToShoot =
		sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::Space) || sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);
	if(wantsToShoot && ownPlayer.canShoot())
	{
		if(m_ammoDisplay.hasAmmo())
		{
			sf::Packet pkt = createTickedPkt(UnreliablePktType::SHOOT, m_clientTick);
			pkt << aimAngle;
			return pkt;
		}
		m_ammoDisplay.triggerEmptyFlash();
	}

	if(posDelta == sf::Vector2f{0, 0})
		return std::nullopt;

	posDelta *= UNRELIABLE_TICK_HZ / (RENDER_TICK_HZ * posDelta.length());
	m_interp.predictMovement(posDelta);

	if(!bServerTickExpired)
		return std::nullopt;

	sf::Packet pkt = createTickedPkt(UnreliablePktType::MOVE, m_clientTick);
	pkt << m_interp.getCumulativeInputs(m_clientTick);
	pkt << aimAngle;
	return pkt;
}

void WorldClient::draw(sf::RenderWindow &window) const
{
	PlayerState const &ownPlayer = m_players[m_ownPlayerId - 1].getState();
	sf::Vector2f const playerCenter = ownPlayer.getPosition() + PlayerState::logicalDimensions / 2.f;

	sf::View cameraView = m_worldView;
	cameraView.setCenter(cartesianToIso(playerCenter));
	cameraView.zoom(m_zoomLevel);
	window.setView(cameraView);

	window.clear(sf::Color::White);

	m_mapClient.drawGroundTiles(window);

	std::vector<RenderObject> renderQueue;
	renderQueue.reserve(2000);

	m_mapClient.collectWallSprites(renderQueue);

	for(auto const &item : m_items)
	{
		if(!item.getState().isActive())
			continue;
		sf::Vector2f const itemPos = item.getSprite().getPosition();
		renderQueue.push_back({.sortY = itemPos.y, .drawable = &item.getSprite()});
	}

	for(auto const &proj : m_projectiles)
	{
		if(!proj.getState().isActive())
			continue;
		sf::Vector2f const projPos = proj.getShape().getPosition();
		renderQueue.push_back({.sortY = projPos.y, .drawable = &proj.getShape()});
	}

	for(auto const &pc : m_players)
	{
		if(pc.getState().m_id == 0 || !pc.getState().isAlive())
			continue;
		pc.collectRenderObjects(renderQueue, m_ownPlayerId);
	}

	std::sort(renderQueue.begin(), renderQueue.end());

	for(auto const &obj : renderQueue)
		obj.draw(window);

	if(m_ownPlayerId > 0 && m_ownPlayerId <= MAX_PLAYERS)
	{
		auto const &ownPlayerClient = m_players[m_ownPlayerId - 1];
		if(ownPlayerClient.getState().isAlive())
			ownPlayerClient.drawSilhouette(window, 100);
	}

	window.setView(m_hudView);

	window.draw(m_healthBar);
	m_itemBar.draw(window);
	m_powerupPanel.draw(window);
	m_ammoDisplay.draw(window);
	m_pauseMenu.draw(window);
}

void WorldClient::applyNonInterpState(WorldState const &snapshot)
{
	m_state.assignSnappedState(snapshot);

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

	for(auto const &gridPos : snapshot.getDestroyedWallDeltas())
		m_state.getMap().destroyWallAtGridPos(gridPos);
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

				if(keyPress->scancode == sf::Keyboard::Scancode::Equal ||
				   keyPress->scancode == sf::Keyboard::Scancode::NumpadPlus)
				{
					m_zoomLevel = std::max(0.5f, m_zoomLevel - 0.1f);
				}
				else if(keyPress->scancode == sf::Keyboard::Scancode::Hyphen ||
				        keyPress->scancode == sf::Keyboard::Scancode::NumpadMinus)
				{
					m_zoomLevel = std::min(3.0f, m_zoomLevel + 0.1f);
				}
			}
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
		else if(auto const *resizeEvent = event->getIf<sf::Event::Resized>())
		{
			handleResize(resizeEvent->size);
		}
	}
}

void WorldClient::handleResize(sf::Vector2u const newSize)
{
	sf::Vector2f const size(newSize);
	m_worldView.setSize(size);
	m_hudView.setSize(size);
	m_hudView.setCenter(size / 2.f);
}
