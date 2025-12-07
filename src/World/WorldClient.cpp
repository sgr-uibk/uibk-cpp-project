#include "WorldClient.h"

#include <algorithm>
#include <cmath>
#include <spdlog/spdlog.h>

#include "Utilities.h"
#include "ResourceManager.h"
#include "GameConfig.h"
#include "UI/Scoreboard.h"

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

WorldClient::WorldClient(sf::RenderWindow &window, EntityId ownPlayerId, std::array<PlayerState, MAX_PLAYERS> &players,
                         sf::Music *battleMusic)
	: m_bAcceptInput(true), m_window(window), m_state(WorldState::fromTiledMap("map/arena.json")),
	  m_mapClient(m_state.getMap()), m_battleMusic(battleMusic),
	  // Members must be initialized before the constructor body runs,
      // so this initializer needs to convert the PlayerStates to PlayerClients *at compile time*
	  m_players(make_players<MAX_PLAYERS>(players, PLAYER_COLORS)), m_ownPlayerId(ownPlayerId),
	  m_pauseFont(g_assetPathResolver.resolveRelative("Font/LiberationSans-Regular.ttf").string())
{
	for(auto &ps : players)
	{
		if(ps.m_id == 0)
			continue;

		m_state.setPlayer(ps);
		m_players[ps.m_id - 1].applyServerState(ps);
	}
	m_worldView = sf::View(sf::FloatRect({0, 0}, sf::Vector2f(window.getSize())));
	m_hudView = sf::View(m_worldView);
	m_frameClock.start();
	m_tickClock.start();

	// Initialize scoreboard
	m_scoreboard = std::make_unique<Scoreboard>(m_pauseFont);

	sf::Vector2f windowSize(window.getSize());
	float buttonWidth = GameConfig::UI::BUTTON_WIDTH;
	float buttonHeight = GameConfig::UI::BUTTON_HEIGHT;
	float centerX = windowSize.x / 2.f - buttonWidth / 2.f;
	float startY = windowSize.y / 2.f - 100.f;
	float spacing = GameConfig::UI::BUTTON_SPACING;

	const std::vector<std::string> buttonLabels = {GameConfig::UI::Text::BUTTON_RESUME,
	                                               GameConfig::UI::Text::BUTTON_DISCONNECT,
	                                               GameConfig::UI::Text::BUTTON_SETTINGS};
	for(size_t i = 0; i < buttonLabels.size(); ++i)
	{
		sf::RectangleShape button(sf::Vector2f(buttonWidth, buttonHeight));
		button.setPosition(sf::Vector2f(centerX, startY + i * (buttonHeight + spacing)));
		button.setFillColor(sf::Color(80, 80, 80));
		button.setOutlineColor(sf::Color::White);
		button.setOutlineThickness(2.f);
		m_pauseButtons.push_back(button);

		sf::Text text(m_pauseFont);
		text.setString(buttonLabels[i]);
		text.setCharacterSize(24);
		text.setFillColor(sf::Color::White);

		sf::FloatRect textBounds = text.getLocalBounds();
		text.setPosition(
			sf::Vector2f(centerX + buttonWidth / 2.f - textBounds.size.x / 2.f,
		                 startY + i * (buttonHeight + spacing) + buttonHeight / 2.f - textBounds.size.y / 2.f - 5.f));
		m_pauseButtonTexts.push_back(text);
	}
}

WorldClient::~WorldClient() = default;

std::optional<sf::Packet> WorldClient::update()
{
	float const frameDelta = m_frameClock.restart().asSeconds();

	// if paused -> skip game logic
	if(m_isPaused)
	{
		// still update animations/visuals but don't send input
		for(auto &pc : m_players)
			pc.update(frameDelta);
		for(auto &proj : m_projectiles)
			proj.update(frameDelta);
		for(auto &item : m_items)
			item.update(frameDelta);
		return std::nullopt;
	}

	// only check keyboard input if this window has focus
	bool const hasFocus = m_window.hasFocus();
	bool const w = hasFocus && sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::W);
	bool const s = hasFocus && sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::S);
	bool const a = hasFocus && sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::A);
	bool const d = hasFocus && sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::D);
	bool const shoot = hasFocus && (sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::Space) ||
	                                sf::Mouse::isButtonPressed(sf::Mouse::Button::Left));

	const sf::Vector2f posDelta{static_cast<float>(d - a), static_cast<float>(s - w)};

	sf::Angle aimAngle;
	if(hasFocus)
	{
		const PlayerState &ps = m_state.getPlayerById(m_ownPlayerId);
		sf::Vector2f playerCartCenter = ps.getPosition() + sf::Vector2f(PlayerState::logicalDimensions / 2.f);

		sf::View cameraView = m_worldView;
		sf::Vector2f isoPlayerPos = cartesianToIso(playerCartCenter);
		cameraView.setCenter(isoPlayerPos);
		cameraView.zoom(m_zoomLevel);

		sf::Vector2i mousePixelPos = sf::Mouse::getPosition(m_window);
		sf::Vector2f mouseIsoPos = m_window.mapPixelToCoords(mousePixelPos, cameraView);

		sf::Vector2f mouseCartPos = isoToCartesian(mouseIsoPos);

		sf::Vector2f dir = mouseCartPos - playerCartCenter;
		float angleRad = std::atan2(dir.y, dir.x);
		float angleDeg = angleRad * 180.0f / 3.14159265f;

		while(angleDeg < 0.0f)
			angleDeg += 360.0f;
		while(angleDeg >= 360.0f)
			angleDeg -= 360.0f;

		angleDeg += 90.0f;

		aimAngle = sf::degrees(angleDeg);
	}
	else
	{
		// fallback to current rotation when not focused
		aimAngle = m_state.getPlayerById(m_ownPlayerId).getRotation();
	}

	for(auto &pc : m_players)
		pc.update(frameDelta);

	for(auto &proj : m_projectiles)
		proj.update(frameDelta);

	for(auto &item : m_items)
		item.update(frameDelta);

	// TODO: Disabled to explicitly show the server delay
	// m_players[m_ownPlayerId-1].applyLocalMove(m_state.map(), posDelta);
	// m_sprite.setRotation();

	if(m_useItemRequested && m_bAcceptInput)
	{
		m_useItemRequested = false;
		sf::Packet pkt = createPkt(UnreliablePktType::USE_ITEM);
		pkt << m_ownPlayerId;
		return std::optional(pkt);
	}

	if(m_slotChangeRequested && m_bAcceptInput)
	{
		m_slotChangeRequested = false;
		sf::Packet pkt = createPkt(UnreliablePktType::SELECT_SLOT);
		pkt << m_ownPlayerId;
		pkt << static_cast<int32_t>(m_requestedSlot);
		return std::optional(pkt);
	}

	if(shoot && m_bAcceptInput && m_state.getPlayerById(m_ownPlayerId).canShoot())
	{
		sf::Packet pkt = createPkt(UnreliablePktType::SHOOT);
		pkt << m_ownPlayerId;
		pkt << aimAngle;
		return std::optional(pkt);
	}

	if(posDelta != sf::Vector2f{0, 0} && m_bAcceptInput &&
	   m_tickClock.getElapsedTime() > sf::seconds(UNRELIABLE_TICK_RATE))
	{
		sf::Packet pkt = createPkt(UnreliablePktType::MOVE);
		pkt << m_ownPlayerId;
		pkt << posDelta;
		pkt << aimAngle;
		m_tickClock.restart();
		return std::optional(pkt);
	}

	// if not moving but mouse aim changed, send rotation update
	if(hasFocus && m_bAcceptInput && m_tickClock.getElapsedTime() > sf::seconds(UNRELIABLE_TICK_RATE))
	{
		sf::Packet pkt = createPkt(UnreliablePktType::MOVE);
		pkt << m_ownPlayerId;
		pkt << sf::Vector2f{0, 0};
		pkt << aimAngle;
		m_tickClock.restart();
		return std::optional(pkt);
	}

	return std::nullopt;
}

void WorldClient::draw(sf::RenderWindow &window) const
{
	sf::View cameraView = m_worldView;
	const PlayerState &ownPlayer = m_state.getPlayerById(m_ownPlayerId);
	sf::Vector2f playerPos = ownPlayer.getPosition();

	sf::Vector2f playerCenter = playerPos + sf::Vector2f(PlayerState::logicalDimensions / 2.f);
	sf::Vector2f isoPlayerPos = cartesianToIso(playerCenter);

	cameraView.setCenter(isoPlayerPos);
	cameraView.zoom(m_zoomLevel);

	window.setView(cameraView);
	window.clear(sf::Color::White);

	sf::RectangleShape gameBackground{sf::Vector2f(WINDOW_DIMf)};
	gameBackground.setFillColor(sf::Color(200, 200, 200));
	window.draw(gameBackground);

	// Draw ground layer first
	m_mapClient.drawGroundTiles(window);

	std::vector<RenderObject> renderQueue;
	renderQueue.reserve(2000); // Pre-allocate space for better performance

	m_mapClient.collectWallSprites(renderQueue);

	for(auto const &pc : m_players)
	{
		if(pc.getState().m_id != 0 && pc.getState().isAlive())
		{
			pc.collectRenderObjects(renderQueue);
		}
	}

	for(auto const &item : m_items)
	{
		if(item.getState().isActive())
		{
			RenderObject obj;
			sf::Vector2f pos = item.getShape().getPosition();
			obj.sortY = pos.y;
			obj.drawable = &item.getShape();
			renderQueue.push_back(obj);
		}
	}

	for(auto const &proj : m_projectiles)
	{
		if(proj.getState().isActive())
		{
			RenderObject obj;
			sf::Vector2f pos = proj.getShape().getPosition();
			obj.sortY = pos.y;
			obj.drawable = &proj.getShape();
			renderQueue.push_back(obj);
		}
	}

	std::sort(renderQueue.begin(), renderQueue.end());

	for(const auto &obj : renderQueue)
	{
		obj.draw(window);
	}

	// visible border around game area
	sf::RectangleShape borderOutline{sf::Vector2f(WINDOW_DIMf)};
	borderOutline.setFillColor(sf::Color::Transparent);
	borderOutline.setOutlineColor(sf::Color::Black);
	borderOutline.setOutlineThickness(3.f);
	window.draw(borderOutline);

	// HUD
	window.setView(m_hudView);
	drawHotbar(window);

	if(m_scoreboard->isShowing())
	{
		m_scoreboard->draw(window);
	}
	else if(m_isPaused)
	{
		drawPauseMenu(window);
	}
}

void WorldClient::applyServerSnapshot(const WorldState &snapshot)
{
	m_state = snapshot;

	const auto &deltas = snapshot.getDestroyedWallDeltas();

	for(const auto &[gridX, gridY] : deltas)
	{
		m_state.getMap().destroyWallAtGridPos(gridX, gridY);

		float expectedX = static_cast<float>(gridX) * CARTESIAN_TILE_SIZE;
		float expectedY = static_cast<float>(gridY) * CARTESIAN_TILE_SIZE;

		auto &walls = m_state.getMap().getWalls();
		for(auto &wall : walls)
		{
			if(wall.isDestroyed())
				continue;

			sf::Vector2f pos = wall.getShape().getPosition();

			if(std::abs(pos.x - expectedX) < 1.0f && std::abs(pos.y - expectedY) < 1.0f)
			{
				wall.destroy();
				spdlog::info("Client: Applied Wall Delta at Grid({}, {})", gridX, gridY);
				break;
			}
		}
	}

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

void WorldClient::pollEvents()
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
			{
				// toggle pause (or return to main pause menu if in settings)
				if(m_isPaused && m_pauseMenuState == PauseMenuState::SETTINGS)
				{
					m_pauseMenuState = PauseMenuState::MAIN;
					spdlog::info("Returned to main pause menu");
				}
				else
				{
					m_isPaused = !m_isPaused;
					if(m_isPaused)
						m_pauseMenuState = PauseMenuState::MAIN;
					spdlog::info("Game {}", m_isPaused ? "paused" : "resumed");
				}
			}
			else if(!m_isPaused && m_window.hasFocus() && m_bAcceptInput)
			{
				if(keyPressed->scancode == sf::Keyboard::Scancode::Q)
				{
					m_useItemRequested = true;
				}
				else if(keyPressed->scancode == sf::Keyboard::Scancode::Equal ||
				        keyPressed->scancode == sf::Keyboard::Scancode::NumpadPlus)
				{
					// Zoom in: +
					m_zoomLevel = std::max(0.5f, m_zoomLevel - 0.1f);
				}
				else if(keyPressed->scancode == sf::Keyboard::Scancode::Hyphen ||
				        keyPressed->scancode == sf::Keyboard::Scancode::NumpadMinus)
				{
					// Zoom out: -
					m_zoomLevel = std::min(2.0f, m_zoomLevel + 0.1f);
				}
				else if(keyPressed->scancode >= sf::Keyboard::Scancode::Num1 &&
				        keyPressed->scancode <= sf::Keyboard::Scancode::Num9)
				{
					int slot = static_cast<int>(keyPressed->scancode) - static_cast<int>(sf::Keyboard::Scancode::Num1);
					m_state.getPlayerById(m_ownPlayerId).setSelectedSlot(slot);
					m_slotChangeRequested = true;
					m_requestedSlot = slot;
				}
			}
		}
		else if(const auto *mouseButtonPressed = event->getIf<sf::Event::MouseButtonPressed>())
		{
			if(mouseButtonPressed->button == sf::Mouse::Button::Left)
			{
				sf::Vector2f mousePos = m_window.mapPixelToCoords(mouseButtonPressed->position, m_hudView);
				if(m_scoreboard->isShowing())
				{
					auto action = m_scoreboard->handleClick(mousePos);
					if(action == Scoreboard::ButtonAction::RETURN_TO_LOBBY)
					{
						m_shouldReturnToLobby = true;
					}
					else if(action == Scoreboard::ButtonAction::MAIN_MENU)
					{
						m_shouldDisconnect = true;
					}
				}
				else if(m_isPaused)
				{
					handlePauseMenuClick(mousePos);
				}
			}
		}
		else if(event->is<sf::Event::FocusLost>())
			m_bAcceptInput = false;
		else if(event->is<sf::Event::FocusGained>())
			m_bAcceptInput = true;
		else if(const auto *mouseWheelScrolled = event->getIf<sf::Event::MouseWheelScrolled>())
		{
			if(m_window.hasFocus() && !m_isPaused)
			{
				int currentSlot = m_state.getPlayerById(m_ownPlayerId).getSelectedSlot();
				if(mouseWheelScrolled->delta > 0)
				{
					currentSlot = (currentSlot - 1 + 9) % 9;
				}
				else if(mouseWheelScrolled->delta < 0)
				{
					currentSlot = (currentSlot + 1) % 9;
				}
				m_state.getPlayerById(m_ownPlayerId).setSelectedSlot(currentSlot);
				m_slotChangeRequested = true;
				m_requestedSlot = currentSlot;
			}
		}
		else if(const auto *resized = event->getIf<sf::Event::Resized>())
		{
			handleResize(resized->size);
		}
	}
}

void WorldClient::drawPauseMenu(sf::RenderWindow &window) const
{
	window.setView(window.getDefaultView());
	sf::RectangleShape overlay(sf::Vector2f(window.getSize()));
	overlay.setFillColor(sf::Color(0, 0, 0, 180));
	window.draw(overlay);

	window.setView(m_hudView);

	if(m_pauseMenuState == PauseMenuState::MAIN)
	{
		sf::Text pauseTitle(m_pauseFont);
		pauseTitle.setString(GameConfig::UI::Text::PAUSE_TITLE);
		pauseTitle.setCharacterSize(48);
		pauseTitle.setFillColor(sf::Color::White);
		sf::FloatRect titleBounds = pauseTitle.getLocalBounds();
		pauseTitle.setPosition(sf::Vector2f(WINDOW_DIM.x / 2.f - titleBounds.size.x / 2.f, 80.f));
		window.draw(pauseTitle);

		sf::Text warningText(m_pauseFont);
		warningText.setString(GameConfig::UI::Text::PAUSE_WARNING);
		warningText.setCharacterSize(20);
		warningText.setFillColor(sf::Color(255, 100, 100));
		sf::FloatRect warningBounds = warningText.getLocalBounds();
		warningText.setPosition(sf::Vector2f(WINDOW_DIM.x / 2.f - warningBounds.size.x / 2.f, 160.f));
		window.draw(warningText);

		for(size_t i = 0; i < m_pauseButtons.size(); ++i)
		{
			window.draw(m_pauseButtons[i]);
			window.draw(m_pauseButtonTexts[i]);
		}
	}
	else if(m_pauseMenuState == PauseMenuState::SETTINGS)
	{
		sf::Text settingsTitle(m_pauseFont);
		settingsTitle.setString(GameConfig::UI::Text::SETTINGS_TITLE);
		settingsTitle.setCharacterSize(40);
		settingsTitle.setFillColor(sf::Color::White);
		sf::FloatRect titleBounds = settingsTitle.getLocalBounds();
		settingsTitle.setPosition(sf::Vector2f(WINDOW_DIM.x / 2.f - titleBounds.size.x / 2.f, 60.f));
		window.draw(settingsTitle);

		if(m_battleMusic)
		{
			sf::RectangleShape musicButton;
			musicButton.setSize({GameConfig::UI::MUSIC_BUTTON_WIDTH, GameConfig::UI::MUSIC_BUTTON_HEIGHT});
			musicButton.setPosition({WINDOW_DIM.x / 2.f - GameConfig::UI::MUSIC_BUTTON_WIDTH / 2.f, 120.f});
			musicButton.setFillColor(sf::Color(80, 80, 80));
			window.draw(musicButton);

			sf::Text musicText(m_pauseFont);
			bool musicPlaying = (m_battleMusic->getStatus() == sf::Music::Status::Playing);
			musicText.setString(musicPlaying ? GameConfig::UI::Text::MUSIC_ON : GameConfig::UI::Text::MUSIC_OFF);
			musicText.setCharacterSize(22);
			musicText.setFillColor(sf::Color::White);
			sf::FloatRect musicBounds = musicText.getLocalBounds();
			musicText.setPosition(sf::Vector2f(
				musicButton.getPosition().x + (GameConfig::UI::MUSIC_BUTTON_WIDTH - musicBounds.size.x) / 2.f -
					musicBounds.position.x,
				musicButton.getPosition().y + (GameConfig::UI::MUSIC_BUTTON_HEIGHT - musicBounds.size.y) / 2.f -
					musicBounds.position.y));
			window.draw(musicText);
		}

		sf::Text keybindsHeader(m_pauseFont);
		keybindsHeader.setString(GameConfig::UI::Text::KEYBINDS_HEADER);
		keybindsHeader.setCharacterSize(28);
		keybindsHeader.setFillColor(sf::Color(200, 200, 100));
		sf::FloatRect headerBounds = keybindsHeader.getLocalBounds();
		keybindsHeader.setPosition(sf::Vector2f(WINDOW_DIM.x / 2.f - headerBounds.size.x / 2.f, 190.f));
		window.draw(keybindsHeader);

		std::vector<std::string> keybindLabels = {
			GameConfig::UI::Text::KEYBIND_MOVE, GameConfig::UI::Text::KEYBIND_SHOOT,
			GameConfig::UI::Text::KEYBIND_USE_ITEM, GameConfig::UI::Text::KEYBIND_SELECT_HOTBAR,
			GameConfig::UI::Text::KEYBIND_PAUSE};

		for(size_t i = 0; i < keybindLabels.size(); ++i)
		{
			sf::Text keybindText(m_pauseFont);
			keybindText.setString(keybindLabels[i]);
			keybindText.setCharacterSize(20);
			keybindText.setFillColor(sf::Color(200, 200, 200));
			sf::FloatRect textBounds = keybindText.getLocalBounds();
			keybindText.setPosition(sf::Vector2f(WINDOW_DIM.x / 2.f - textBounds.size.x / 2.f, 235.f + i * 32.f));
			window.draw(keybindText);
		}

		sf::RectangleShape backButton;
		backButton.setSize({GameConfig::UI::BUTTON_WIDTH, GameConfig::UI::BUTTON_HEIGHT});
		backButton.setPosition(
			{WINDOW_DIM.x / 2.f - GameConfig::UI::BUTTON_WIDTH / 2.f, static_cast<float>(WINDOW_DIM.y) - 120.f});
		backButton.setFillColor(sf::Color(80, 80, 80));
		window.draw(backButton);

		sf::Text backText(m_pauseFont);
		backText.setString(GameConfig::UI::Text::BUTTON_BACK);
		backText.setCharacterSize(24);
		backText.setFillColor(sf::Color::White);
		sf::FloatRect backBounds = backText.getLocalBounds();
		backText.setPosition(
			sf::Vector2f(backButton.getPosition().x + (GameConfig::UI::BUTTON_WIDTH - backBounds.size.x) / 2.f -
		                     backBounds.position.x,
		                 backButton.getPosition().y + (GameConfig::UI::BUTTON_HEIGHT - backBounds.size.y) / 2.f -
		                     backBounds.position.y));
		window.draw(backText);
	}
}

void WorldClient::handlePauseMenuClick(sf::Vector2f mousePos)
{
	if(m_pauseMenuState == PauseMenuState::MAIN)
	{
		for(size_t i = 0; i < m_pauseButtons.size(); ++i)
		{
			if(m_pauseButtons[i].getGlobalBounds().contains(mousePos))
			{
				if(i == 0)
				{
					m_isPaused = false;
					spdlog::info("Resumed game");
				}
				else if(i == 1)
				{
					m_shouldDisconnect = true;
					spdlog::info("Disconnecting from game");
				}
				else if(i == 2)
				{
					m_pauseMenuState = PauseMenuState::SETTINGS;
					spdlog::info("Entered pause menu settings");
				}
				break;
			}
		}
	}
	else if(m_pauseMenuState == PauseMenuState::SETTINGS)
	{
		if(m_battleMusic)
		{
			sf::FloatRect musicButtonBounds(
				sf::Vector2f(WINDOW_DIM.x / 2.f - GameConfig::UI::MUSIC_BUTTON_WIDTH / 2.f, 120.f),
				sf::Vector2f(GameConfig::UI::MUSIC_BUTTON_WIDTH, GameConfig::UI::MUSIC_BUTTON_HEIGHT));

			if(musicButtonBounds.contains(mousePos))
			{
				if(m_battleMusic->getStatus() == sf::Music::Status::Playing)
				{
					m_battleMusic->stop();
					spdlog::info("Battle music stopped");
				}
				else
				{
					m_battleMusic->play();
					spdlog::info("Battle music started");
				}
				return;
			}
		}

		// if back button clicked
		sf::FloatRect backButtonBounds(sf::Vector2f(WINDOW_DIM.x / 2.f - GameConfig::UI::BUTTON_WIDTH / 2.f,
		                                            static_cast<float>(WINDOW_DIM.y) - 120.f),
		                               sf::Vector2f(GameConfig::UI::BUTTON_WIDTH, GameConfig::UI::BUTTON_HEIGHT));

		if(backButtonBounds.contains(mousePos))
		{
			m_pauseMenuState = PauseMenuState::MAIN;
			spdlog::info("Returned to main pause menu");
		}
	}
}

void WorldClient::drawHotbar(sf::RenderWindow &window) const
{
	const PlayerState &ownPlayer = m_state.getPlayerById(m_ownPlayerId);
	const auto &inventory = ownPlayer.getInventory();
	int selectedSlot = ownPlayer.getSelectedSlot();

	const float slotSize = GameConfig::UI::HOTBAR_SLOT_SIZE;
	const float slotSpacing = GameConfig::UI::HOTBAR_SLOT_SPACING;
	const float hotbarWidth = 9 * slotSize + 8 * slotSpacing;
	const sf::Vector2f windowSize = sf::Vector2f(WINDOW_DIM);
	const float startX = windowSize.x / 2.f - hotbarWidth / 2.f;
	const float startY = windowSize.y - slotSize - 20.f;

	auto getColorForPowerup = [](PowerupType type) -> sf::Color {
		switch(type)
		{
		case PowerupType::HEALTH_PACK:
			return sf::Color::Green;
		case PowerupType::SPEED_BOOST:
			return sf::Color::Cyan;
		case PowerupType::DAMAGE_BOOST:
			return sf::Color::Red;
		case PowerupType::SHIELD:
			return sf::Color::Blue;
		case PowerupType::RAPID_FIRE:
			return sf::Color::Yellow;
		case PowerupType::NONE:
		default:
			return sf::Color(60, 60, 60);
		}
	};

	auto getNameForPowerup = [](PowerupType type) -> std::string {
		switch(type)
		{
		case PowerupType::HEALTH_PACK:
			return "HP";
		case PowerupType::SPEED_BOOST:
			return "SPD";
		case PowerupType::DAMAGE_BOOST:
			return "DMG";
		case PowerupType::SHIELD:
			return "SHD";
		case PowerupType::RAPID_FIRE:
			return "RPD";
		case PowerupType::NONE:
		default:
			return "";
		}
	};

	for(int i = 0; i < 9; ++i)
	{
		float x = startX + i * (slotSize + slotSpacing);

		// slot background
		sf::RectangleShape slot(sf::Vector2f(slotSize, slotSize));
		slot.setPosition(sf::Vector2f(x, startY));
		slot.setFillColor(sf::Color(40, 40, 40, 200));

		// selected slot highlight
		if(i == selectedSlot)
		{
			slot.setOutlineColor(sf::Color::White);
			slot.setOutlineThickness(3.f);
		}
		else
		{
			slot.setOutlineColor(sf::Color(100, 100, 100));
			slot.setOutlineThickness(1.f);
		}
		window.draw(slot);

		PowerupType itemType = inventory[i];
		if(itemType != PowerupType::NONE)
		{
			sf::RectangleShape itemIcon(sf::Vector2f(slotSize - 10.f, slotSize - 10.f));
			itemIcon.setPosition(sf::Vector2f(x + 5.f, startY + 5.f));
			itemIcon.setFillColor(getColorForPowerup(itemType));
			window.draw(itemIcon);

			sf::Text itemText(m_pauseFont);
			itemText.setString(getNameForPowerup(itemType));
			itemText.setCharacterSize(12);
			itemText.setFillColor(sf::Color::White);
			sf::FloatRect textBounds = itemText.getLocalBounds();
			itemText.setPosition(sf::Vector2f(x + slotSize / 2.f - textBounds.size.x / 2.f,
			                                  startY + slotSize / 2.f - textBounds.size.y / 2.f - 3.f));
			window.draw(itemText);
		}

		sf::Text slotNumber(m_pauseFont);
		slotNumber.setString(std::to_string(i + 1));
		slotNumber.setCharacterSize(14);
		slotNumber.setFillColor(sf::Color(200, 200, 200));
		sf::FloatRect numBounds = slotNumber.getLocalBounds();
		slotNumber.setPosition(sf::Vector2f(x + 5.f, startY + 2.f));
		window.draw(slotNumber);
	}
}
void WorldClient::showScoreboard(EntityId winnerId, const std::vector<PlayerStats> &playerStats)
{
	m_bAcceptInput = false;

	std::vector<Scoreboard::PlayerStats> scoreboardStats;
	for(const auto &stats : playerStats)
	{
		scoreboardStats.push_back({stats.id, stats.name, stats.kills, stats.deaths});
	}

	m_scoreboard->show(winnerId, scoreboardStats);
}

void WorldClient::handleResize(sf::Vector2u newSize)
{
	spdlog::info("Window resized to {}x{}", newSize.x, newSize.y);

	// uniform scaling
	float scaleX = static_cast<float>(newSize.x) / static_cast<float>(WINDOW_DIM.x);
	float scaleY = static_cast<float>(newSize.y) / static_cast<float>(WINDOW_DIM.y);
	float scale = std::min(scaleX, scaleY);

	float viewportWidth = (WINDOW_DIM.x * scale) / newSize.x;
	float viewportHeight = (WINDOW_DIM.y * scale) / newSize.y;
	float viewportX = (1.f - viewportWidth) / 2.f;
	float viewportY = (1.f - viewportHeight) / 2.f;

	m_worldView = sf::View(sf::FloatRect({0.f, 0.f}, sf::Vector2f(WINDOW_DIM)));
	m_hudView = sf::View(sf::FloatRect({0.f, 0.f}, sf::Vector2f(WINDOW_DIM)));

	m_worldView.setViewport(sf::FloatRect({viewportX, viewportY}, {viewportWidth, viewportHeight}));
	m_hudView.setViewport(sf::FloatRect({viewportX, viewportY}, {viewportWidth, viewportHeight}));
}
