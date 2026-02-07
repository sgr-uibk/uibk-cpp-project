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
                         std::array<PlayerState, MAX_PLAYERS> players, bool gameMusicEnabled)
	: m_bAcceptInput(true), m_pauseMenu(window, gameMusicEnabled), m_state(mapIndex, std::move(players)),
	  m_itemBar(m_state.getPlayerById(ownPlayerId), window),
	  m_healthBar(sf::Vector2f(20.f, 20.f), sf::Vector2f(220.f, 28.f),
                  m_state.getPlayerById(ownPlayerId).getMaxHealth()),
	  m_powerupPanel(m_state.getPlayerById(ownPlayerId), window),
	  m_ammoDisplay(m_state.getPlayerById(ownPlayerId), window),
	  m_minimap(m_state.getMap().getSize(), sf::Vector2f(window.getSize())), m_window(window),
	  m_mapClient(m_state.getMap()), m_players(make_players<MAX_PLAYERS>(m_state.m_players, PLAYER_COLORS)),
	  m_ownPlayerId(ownPlayerId), m_interp(m_state.getMap(), ownPlayerId, m_players)
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
	m_healthBar.update(dt);
}

bool WorldClient::update(WorldUpdateData &wud)
{
	float const frameDelta = m_frameClock.restart().asSeconds();
	m_clientTick++;
	bool const bServerTickExpired = m_tickClock.getElapsedTime().asSeconds() >= UNRELIABLE_TICK_TIME;
	if(bServerTickExpired)
		m_tickClock.restart();
	propagateUpdate(frameDelta);

	// Update healthbar
	PlayerState const &ownPlayerState = m_state.getPlayerById(m_ownPlayerId);
	m_healthBar.setHealth(ownPlayerState.getHealth());
	m_healthBar.setMaxHealth(ownPlayerState.getMaxHealth());

	if(m_pauseMenu.isPaused() || !m_bAcceptInput || !m_window.hasFocus())
		return false;

	PlayerState const &ps = m_state.getPlayerById(m_ownPlayerId);
	sf::Vector2f playerCartCenter = ps.getPosition() + sf::Vector2f(PlayerState::logicalDimensions / 2.f);

	sf::View cameraView = m_worldView;
	cameraView.setCenter(cartesianToIso(playerCartCenter));
	cameraView.zoom(m_zoomLevel);

	sf::Vector2i mousePixelPos = sf::Mouse::getPosition(m_window);
	sf::Vector2f mouseIsoPos = m_window.mapPixelToCoords(mousePixelPos, cameraView);
	sf::Vector2f mouseCartPos = isoToCartesian(mouseIsoPos);
	sf::Vector2f dir = mouseCartPos - playerCartCenter;

	wud.cannonRot = (dir.angle() + sf::degrees(90.0f)).wrapUnsigned();

	m_players[m_ownPlayerId - 1].setCannonRotation(wud.cannonRot);

	if(m_itemBar.handleItemUse())
	{
		size_t const slot = m_itemBar.getSelection();
		PlayerState const &ownPlayer = m_state.getPlayerById(m_ownPlayerId);
		PowerupType const itemType = ownPlayer.getInventoryItem(static_cast<int>(slot - 1));

		if(itemType != PowerupType::NONE && !ownPlayer.canUsePowerup(itemType))
			m_powerupPanel.triggerCooldownFlash(itemType);
		else
			wud.slot = slot;
	}

	if(wud.bShoot)
	{
		if(!m_ammoDisplay.hasAmmo())
		{
			m_ammoDisplay.triggerEmptyFlash();
			wud.bShoot = false;
		}
		else if(!m_state.getPlayerById(m_ownPlayerId).canShoot())
		{
			wud.bShoot = false;
		}
		// sound and ammo deduction handled via cooldown detection in PlayerClient and AmmunitionDisplay
	}

	if(wud.posDelta != sf::Vector2f{0, 0})
	{ // now we can safely normalize
		wud.posDelta *= UNRELIABLE_TICK_HZ / (RENDER_TICK_HZ * wud.posDelta.length());
		m_interp.predictMovement(wud.posDelta);
		if(bServerTickExpired)
			wud.posDelta = m_interp.getCumulativeInputs(m_clientTick);
	}
	// Movement is tick-rate-limited, others get sent immediately
	return bServerTickExpired || wud.bShoot || wud.slot;
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

	m_safeZoneClient.update(m_state.m_safeZone, playerCenter);
	window.draw(m_safeZoneClient);

	std::vector<RenderObject> renderQueue;

	m_mapClient.collectWallSprites(renderQueue);

	for(auto const &item : m_items)
	{
		if(item.getState().isActive())
		{
			sf::Vector2f itemPos = item.getSprite().getPosition();
			RenderObject obj;
			obj.sortY = itemPos.y + 40.f;
			obj.drawable = &item.getSprite();
			renderQueue.push_back(obj);
		}
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
		if(pc.getState().m_id == 0)
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
	m_minimap.updatePlayers(m_state.m_players, m_ownPlayerId);
	m_minimap.updateSafeZone(m_state.m_safeZone);
	window.draw(m_minimap);
	m_safeZoneClient.drawDangerOverlay(window, sf::Vector2f(window.getSize()));
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

				// Handle camera zoom with +/- keys
				if(keyPress->scancode == sf::Keyboard::Scancode::Equal ||
				   keyPress->scancode == sf::Keyboard::Scancode::NumpadPlus)
				{
					m_zoomLevel = std::max(0.5f, m_zoomLevel - 0.1f); // Zoom in (decrease zoom value)
				}
				else if(keyPress->scancode == sf::Keyboard::Scancode::Hyphen ||
				        keyPress->scancode == sf::Keyboard::Scancode::NumpadMinus)
				{
					m_zoomLevel = std::min(3.0f, m_zoomLevel + 0.1f); // Zoom out (increase zoom value)
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

void WorldClient::handleResize(sf::Vector2u newSize)
{
	// Update both views to match new window size
	m_worldView.setSize(sf::Vector2f(newSize));
	m_hudView.setSize(sf::Vector2f(newSize));
	m_hudView.setCenter(sf::Vector2f(newSize) / 2.f);
	m_minimap.updateScreenSize(sf::Vector2f(newSize));
}
