#include "Menu.h"
#include "Lobby/LobbyClient.h"
#include "ResourceManager.h"
#include "Networking.h"
#include "GameConfig.h"
#include <spdlog/spdlog.h>
#include <algorithm>

Menu::Menu(sf::Vector2u windowDimensions)
	: m_windowDimensions(windowDimensions),
	  m_font(g_assetPathResolver.resolveRelative("Font/LiberationSans-Regular.ttf").string()), m_title(m_font),
	  m_playerNameLabel(m_font), m_playerNameText(m_font), m_state(State::MAIN), m_startGame(false), m_exit(false),
	  m_shouldConnect(false), m_playerName("Player"), m_editingName(false), m_menuMusicEnabled(true),
	  m_gameMusicEnabled(true), m_serverIp("127.0.0.1"), m_serverPort(PORT_TCP), m_serverIpText(m_font),
	  m_serverPortText(m_font), m_editingField(EditingField::NONE), m_selectedMap("Arena"), m_selectedMode("Deathmatch")
{
	m_title.setCharacterSize(50);
	m_title.setFillColor(sf::Color::White);
	setTitle("TANK GAME");

	m_playerNameLabel.setString("Player Name:");
	m_playerNameLabel.setCharacterSize(18);
	m_playerNameLabel.setFillColor(sf::Color::White);
	m_playerNameLabel.setPosition({50.f, windowDimensions.y - 60.f});

	m_playerNameBox.setSize({150.f, 30.f});
	m_playerNameBox.setPosition({180.f, windowDimensions.y - 65.f});
	m_playerNameBox.setFillColor(sf::Color(40, 40, 40));
	m_playerNameBox.setOutlineColor(sf::Color(100, 100, 100));
	m_playerNameBox.setOutlineThickness(2.f);

	m_playerNameText.setCharacterSize(18);
	m_playerNameText.setFillColor(sf::Color::White);
	m_playerNameText.setPosition({185.f, windowDimensions.y - 60.f});

	updatePlayerNameDisplay();
	setupMainMenu();
}

void Menu::updatePlayerNameDisplay()
{
	std::string displayText = m_playerName;
	if(m_editingField == EditingField::PLAYER_NAME)
		displayText += "_";
	m_playerNameText.setString(displayText);
}

void Menu::reset()
{
	m_startGame = false;
	m_exit = false;
	m_shouldConnect = false;
	m_state = State::MAIN;
	setTitle("TANK GAME");
	setupMainMenu();
}

void Menu::setTitle(std::string const &title)
{
	m_title.setString(title);
	sf::FloatRect titleBounds = m_title.getLocalBounds();
	m_title.setPosition({(m_windowDimensions.x - titleBounds.size.x) / 2.f - titleBounds.position.x, 80.f});
}

void Menu::clearMenuState()
{
	m_buttonShapes.clear();
	m_buttonTexts.clear();
	m_buttonEnabled.clear();
	m_lobbyTexts.clear();
}

void Menu::centerTextInButton(sf::Text &text, sf::RectangleShape const &button)
{
	sf::FloatRect textBounds = text.getLocalBounds();
	sf::Vector2f buttonSize = button.getSize();
	sf::Vector2f buttonPos = button.getPosition();
	text.setPosition({buttonPos.x + (buttonSize.x - textBounds.size.x) / 2.f - textBounds.position.x,
	                  buttonPos.y + (buttonSize.y - textBounds.size.y) / 2.f - textBounds.position.y});
}

void Menu::addButton(sf::Vector2f position, sf::Vector2f size, std::string const &label, unsigned int fontSize,
                     sf::Color fillColor)
{
	sf::RectangleShape shape;
	shape.setSize(size);
	shape.setPosition(position);
	shape.setFillColor(fillColor);
	m_buttonShapes.push_back(shape);

	sf::Text text(m_font);
	text.setString(label);
	text.setCharacterSize(fontSize);
	text.setFillColor(sf::Color::White);
	centerTextInButton(text, shape);
	m_buttonTexts.push_back(text);
	m_buttonEnabled.push_back(true);
}

void Menu::goToMainMenu()
{
	m_state = State::MAIN;
	setTitle("TANK GAME");
	setupMainMenu();
}

void Menu::setupMainMenu()
{
	clearMenuState();

	float const buttonWidth = 280.f;
	float const buttonHeight = 45.f;
	float const spacing = 15.f;
	float const startY = 200.f;
	float const centerX = (m_windowDimensions.x - buttonWidth) / 2.f;

	std::vector<std::string> labels = {"Join Lobby", "Settings", "Exit"};

	for(size_t i = 0; i < labels.size(); ++i)
	{
		addButton({centerX, startY + i * (buttonHeight + spacing)}, {buttonWidth, buttonHeight}, labels[i], 20);
	}
}

void Menu::setupLobbyHost()
{
	clearMenuState();

	float const buttonWidth = 200.f;
	float const buttonHeight = 50.f;
	float const smallButtonWidth = 180.f;
	float const smallButtonHeight = 35.f;
	float const centerX = (m_windowDimensions.x - buttonWidth) / 2.f;

	sf::Text lobbyInfo(m_font);
	lobbyInfo.setString("Server running on port " + std::to_string(PORT_TCP));
	lobbyInfo.setCharacterSize(20);
	lobbyInfo.setFillColor(sf::Color(200, 200, 200));
	sf::FloatRect infoBounds = lobbyInfo.getLocalBounds();
	lobbyInfo.setPosition({(m_windowDimensions.x - infoBounds.size.x) / 2.f - infoBounds.position.x, 180.f});
	m_lobbyTexts.push_back(lobbyInfo);

	sf::Text playersLabel(m_font);
	playersLabel.setString("Players (1/4):");
	playersLabel.setCharacterSize(24);
	playersLabel.setFillColor(sf::Color::White);
	playersLabel.setPosition({200.f, 240.f});
	m_lobbyTexts.push_back(playersLabel);

	sf::Text player1(m_font);
	player1.setString("1. [Ready] " + m_playerName + " (Host)");
	player1.setCharacterSize(20);
	player1.setFillColor(sf::Color(100, 255, 100));
	player1.setPosition({220.f, 280.f});
	m_lobbyTexts.push_back(player1);

	for(int i = 2; i <= 4; ++i)
	{
		sf::Text playerSlot(m_font);
		playerSlot.setString(std::to_string(i) + ". Waiting...");
		playerSlot.setCharacterSize(20);
		playerSlot.setFillColor(sf::Color(150, 150, 150));
		playerSlot.setPosition({220.f, 280.f + (i - 1) * 30.f});
		m_lobbyTexts.push_back(playerSlot);
	}

	sf::Text mapLabel(m_font);
	mapLabel.setString("Map:");
	mapLabel.setCharacterSize(18);
	mapLabel.setFillColor(sf::Color::White);
	mapLabel.setPosition({50.f, 410.f});
	m_lobbyTexts.push_back(mapLabel);

	addButton({120.f, 405.f}, {smallButtonWidth, smallButtonHeight}, m_selectedMap, 16, sf::Color(60, 60, 100));

	sf::Text modeLabel(m_font);
	modeLabel.setString("Mode:");
	modeLabel.setCharacterSize(18);
	modeLabel.setFillColor(sf::Color::White);
	modeLabel.setPosition({320.f, 410.f});
	m_lobbyTexts.push_back(modeLabel);

	addButton({390.f, 405.f}, {smallButtonWidth, smallButtonHeight}, m_selectedMode, 16, sf::Color(60, 100, 60));

	addButton({centerX - 110.f, 450.f}, {buttonWidth, buttonHeight}, "Start Game", 24);
	addButton({centerX + 110.f, 450.f}, {buttonWidth, buttonHeight}, "Back", 24);
}

void Menu::setupLobbyClient()
{
	clearMenuState();

	float const buttonWidth = 200.f;
	float const buttonHeight = 50.f;
	float const centerX = (m_windowDimensions.x - buttonWidth) / 2.f;

	sf::Text lobbyInfo(m_font);
	lobbyInfo.setString("Waiting in lobby...");
	lobbyInfo.setCharacterSize(20);
	lobbyInfo.setFillColor(sf::Color(200, 200, 200));
	sf::FloatRect infoBounds = lobbyInfo.getLocalBounds();
	lobbyInfo.setPosition({(m_windowDimensions.x - infoBounds.size.x) / 2.f - infoBounds.position.x, 180.f});
	m_lobbyTexts.push_back(lobbyInfo);

	sf::Text playersLabel(m_font);
	playersLabel.setString("Players (1/4):");
	playersLabel.setCharacterSize(24);
	playersLabel.setFillColor(sf::Color::White);
	playersLabel.setPosition({200.f, 240.f});
	m_lobbyTexts.push_back(playersLabel);

	sf::Text player1(m_font);
	player1.setString("1. [Not Ready] " + m_playerName);
	player1.setCharacterSize(20);
	player1.setFillColor(sf::Color(255, 200, 100));
	player1.setPosition({220.f, 280.f});
	m_lobbyTexts.push_back(player1);

	for(int i = 2; i <= 4; ++i)
	{
		sf::Text playerSlot(m_font);
		playerSlot.setString(std::to_string(i) + ". Waiting...");
		playerSlot.setCharacterSize(20);
		playerSlot.setFillColor(sf::Color(150, 150, 150));
		playerSlot.setPosition({220.f, 280.f + (i - 1) * 30.f});
		m_lobbyTexts.push_back(playerSlot);
	}

	addButton({centerX - 110.f, 450.f}, {buttonWidth, buttonHeight}, "Ready", 24);
	addButton({centerX + 110.f, 450.f}, {buttonWidth, buttonHeight}, "Leave", 24);
}

void Menu::setupJoinLobby()
{
	clearMenuState();

	float const buttonWidth = 200.f;
	float const buttonHeight = 50.f;
	float const centerX = (m_windowDimensions.x - buttonWidth) / 2.f;

	sf::Text ipLabel(m_font);
	ipLabel.setString("Server IP:");
	ipLabel.setCharacterSize(20);
	ipLabel.setFillColor(sf::Color::White);
	ipLabel.setPosition({200.f, 220.f});
	m_lobbyTexts.push_back(ipLabel);

	m_serverIpBox.setSize({180.f, 35.f});
	m_serverIpBox.setPosition({320.f, 215.f});
	m_serverIpBox.setFillColor(sf::Color(40, 40, 40));
	m_serverIpBox.setOutlineColor(sf::Color(100, 100, 100));
	m_serverIpBox.setOutlineThickness(2.f);

	m_serverIpText.setCharacterSize(18);
	m_serverIpText.setFillColor(sf::Color::White);
	m_serverIpText.setPosition({325.f, 220.f});
	m_serverIpText.setString(m_serverIp);

	sf::Text portLabel(m_font);
	portLabel.setString("Port:");
	portLabel.setCharacterSize(20);
	portLabel.setFillColor(sf::Color::White);
	portLabel.setPosition({200.f, 270.f});
	m_lobbyTexts.push_back(portLabel);

	m_serverPortBox.setSize({120.f, 35.f});
	m_serverPortBox.setPosition({320.f, 265.f});
	m_serverPortBox.setFillColor(sf::Color(40, 40, 40));
	m_serverPortBox.setOutlineColor(sf::Color(100, 100, 100));
	m_serverPortBox.setOutlineThickness(2.f);

	m_serverPortText.setCharacterSize(18);
	m_serverPortText.setFillColor(sf::Color::White);
	m_serverPortText.setPosition({325.f, 270.f});
	m_serverPortText.setString(std::to_string(m_serverPort));

	addButton({centerX - 110.f, 350.f}, {buttonWidth, buttonHeight}, "Connect", 24);
	addButton({centerX + 110.f, 350.f}, {buttonWidth, buttonHeight}, "Back", 24);
}

void Menu::setupSettings()
{
	clearMenuState();

	float const buttonWidth = 280.f;
	float const buttonHeight = 42.f;
	float const startY = 180.f;
	float const centerX = (m_windowDimensions.x - buttonWidth) / 2.f;

	sf::Text audioHeader(m_font);
	audioHeader.setString("AUDIO SETTINGS");
	audioHeader.setCharacterSize(20);
	audioHeader.setFillColor(sf::Color(200, 200, 100));
	audioHeader.setPosition({(m_windowDimensions.x - 180.f) / 2.f, startY});
	m_lobbyTexts.push_back(audioHeader);

	std::string menuMusicStr = m_menuMusicEnabled ? "Menu Music: ON" : "Menu Music: OFF";
	addButton({centerX, startY + 35.f}, {buttonWidth, buttonHeight}, menuMusicStr, 18);

	std::string gameMusicStr = m_gameMusicEnabled ? "Game Music: ON" : "Game Music: OFF";
	addButton({centerX, startY + 85.f}, {buttonWidth, buttonHeight}, gameMusicStr, 18);

	sf::Text keybindsHeader(m_font);
	keybindsHeader.setString("KEYBINDS");
	keybindsHeader.setCharacterSize(20);
	keybindsHeader.setFillColor(sf::Color(200, 200, 100));
	keybindsHeader.setPosition({(m_windowDimensions.x - 110.f) / 2.f, startY + 145.f});
	m_lobbyTexts.push_back(keybindsHeader);

	std::vector<std::string> keybindLabels = {"Move: W A S D", "Shoot: Space / Click", "Use Item: Q",
	                                          "Hotbar: 1-9 / Wheel", "Pause: Escape"};

	float const leftColX = 150.f;
	float const rightColX = 450.f;
	float const keybindStartY = startY + 175.f;
	float const keybindSpacing = 24.f;

	for(size_t i = 0; i < keybindLabels.size(); ++i)
	{
		sf::Text keybindText(m_font);
		keybindText.setString(keybindLabels[i]);
		keybindText.setCharacterSize(15);
		keybindText.setFillColor(sf::Color(180, 180, 180));

		float xPos = (i < 3) ? leftColX : rightColX;
		float yOffset = (i < 3) ? i : (i - 3);
		keybindText.setPosition({xPos, keybindStartY + yOffset * keybindSpacing});
		m_lobbyTexts.push_back(keybindText);
	}

	addButton({centerX, startY + 255.f}, {buttonWidth, buttonHeight}, "Back", 20);
}

void Menu::setupMapSelection()
{
	clearMenuState();

	float const buttonWidth = 250.f;
	float const buttonHeight = 50.f;
	float const startY = 180.f;
	float const centerX = (m_windowDimensions.x - buttonWidth) / 2.f;

	std::vector<std::string> const maps = {"Test1", "Test2", "Test3", "Test4"};

	for(size_t i = 0; i < maps.size(); ++i)
	{
		sf::Color fillColor = (maps[i] == m_selectedMap) ? sf::Color(60, 120, 60) : sf::Color(80, 80, 80);
		addButton({centerX, startY + i * (buttonHeight + 15.f)}, {buttonWidth, buttonHeight}, maps[i], 22, fillColor);
	}

	addButton({centerX, startY + maps.size() * (buttonHeight + 15.f) + 30.f}, {buttonWidth, buttonHeight}, "Back", 24);
}

void Menu::setupModeSelection()
{
	clearMenuState();

	float const buttonWidth = 250.f;
	float const buttonHeight = 50.f;
	float const startY = 180.f;
	float const centerX = (m_windowDimensions.x - buttonWidth) / 2.f;

	std::vector<std::string> const modes = {"Deathmatch", "TestMode2", "TestMode3", "TestMode4"};

	for(size_t i = 0; i < modes.size(); ++i)
	{
		sf::Color fillColor = (modes[i] == m_selectedMode) ? sf::Color(60, 120, 60) : sf::Color(80, 80, 80);
		addButton({centerX, startY + i * (buttonHeight + 15.f)}, {buttonWidth, buttonHeight}, modes[i], 22, fillColor);
	}

	addButton({centerX, startY + modes.size() * (buttonHeight + 15.f) + 30.f}, {buttonWidth, buttonHeight}, "Back", 24);
}

bool Menu::isMouseOver(sf::RectangleShape const &shape, sf::Vector2f mousePos) const
{
	sf::FloatRect bounds = shape.getGlobalBounds();
	return bounds.contains(mousePos);
}

void Menu::handleMouseMove(sf::Vector2f mousePos)
{
	for(size_t i = 0; i < m_buttonShapes.size(); ++i)
	{
		if(m_buttonEnabled[i] && isMouseOver(m_buttonShapes[i], mousePos))
		{
			m_buttonShapes[i].setFillColor(sf::Color(100, 100, 100));
		}
		else
		{
			m_buttonShapes[i].setFillColor(m_buttonEnabled[i] ? sf::Color(80, 80, 80) : sf::Color(50, 50, 50));
		}
	}

	if(m_state == State::MAIN && isMouseOver(m_playerNameBox, mousePos))
	{
		m_playerNameBox.setOutlineColor(sf::Color(150, 150, 150));
	}
	else if(m_state == State::MAIN)
	{
		m_playerNameBox.setOutlineColor(m_editingField == EditingField::PLAYER_NAME ? sf::Color(200, 200, 100)
		                                                                            : sf::Color(100, 100, 100));
	}

	if(m_state == State::JOIN_LOBBY)
	{
		if(isMouseOver(m_serverIpBox, mousePos))
		{
			m_serverIpBox.setOutlineColor(sf::Color(150, 150, 150));
		}
		else
		{
			m_serverIpBox.setOutlineColor(m_editingField == EditingField::SERVER_IP ? sf::Color(200, 200, 100)
			                                                                        : sf::Color(100, 100, 100));
		}

		if(isMouseOver(m_serverPortBox, mousePos))
		{
			m_serverPortBox.setOutlineColor(sf::Color(150, 150, 150));
		}
		else
		{
			m_serverPortBox.setOutlineColor(m_editingField == EditingField::SERVER_PORT ? sf::Color(200, 200, 100)
			                                                                            : sf::Color(100, 100, 100));
		}
	}
}

void Menu::handleTextInput(char c)
{
	if(m_editingField == EditingField::PLAYER_NAME)
	{
		if(c == '\b')
		{
			if(!m_playerName.empty())
				m_playerName.pop_back();
		}
		else if(c >= ' ' && c <= '~' && m_playerName.length() < 12)
		{
			m_playerName += c;
		}
		updatePlayerNameDisplay();
	}
	else if(m_editingField == EditingField::SERVER_IP)
	{
		if(c == '\b')
		{
			if(!m_serverIp.empty())
				m_serverIp.pop_back();
		}
		else if((c >= '0' && c <= '9') || c == '.')
		{
			if(m_serverIp.length() < 15)
				m_serverIp += c;
		}
		m_serverIpText.setString(m_serverIp);
	}
	else if(m_editingField == EditingField::SERVER_PORT)
	{
		if(c == '\b')
		{
			if(m_serverPort > 0)
			{
				std::string portStr = std::to_string(m_serverPort);
				if(!portStr.empty())
				{
					portStr.pop_back();
					m_serverPort = portStr.empty() ? 0 : std::stoi(portStr);
				}
			}
		}
		else if(c >= '0' && c <= '9')
		{
			std::string portStr = std::to_string(m_serverPort);
			if(m_serverPort == 0)
				portStr = "";
			portStr += c;
			if(portStr.length() <= 5)
			{
				uint32_t newPort = std::stoi(portStr);
				if(newPort <= 65535)
					m_serverPort = static_cast<uint16_t>(newPort);
			}
		}
		m_serverPortText.setString(std::to_string(m_serverPort));
	}
}

void Menu::handleClick(sf::Vector2f mousePos)
{
	if(m_state == State::MAIN && isMouseOver(m_playerNameBox, mousePos))
	{
		m_editingField = (m_editingField == EditingField::PLAYER_NAME) ? EditingField::NONE : EditingField::PLAYER_NAME;
		spdlog::info("Player name field clicked, editing: {}, current name: '{}'",
		             m_editingField == EditingField::PLAYER_NAME, m_playerName);
		updatePlayerNameDisplay();
		return;
	}

	if(m_state == State::JOIN_LOBBY)
	{
		if(isMouseOver(m_serverIpBox, mousePos))
		{
			m_editingField = (m_editingField == EditingField::SERVER_IP) ? EditingField::NONE : EditingField::SERVER_IP;
			return;
		}
		else if(isMouseOver(m_serverPortBox, mousePos))
		{
			m_editingField =
				(m_editingField == EditingField::SERVER_PORT) ? EditingField::NONE : EditingField::SERVER_PORT;
			return;
		}
	}

	for(size_t i = 0; i < m_buttonShapes.size(); ++i)
	{
		if(!m_buttonEnabled[i] || !isMouseOver(m_buttonShapes[i], mousePos))
			continue;

		switch(m_state)
		{
		case State::MAIN:
			if(i == 0)
			{
				spdlog::info("Join Lobby button clicked, player name is: '{}'", m_playerName);
				m_state = State::JOIN_LOBBY;
				setTitle("CONNECT TO SERVER");
				setupJoinLobby();
			}
			else if(i == 1)
			{
				m_state = State::SETTINGS;
				setTitle("SETTINGS");
				setupSettings();
			}
			else if(i == 2)
			{
				m_exit = true;
			}
			break;

		case State::JOIN_LOBBY:
			if(i == 0)
				m_shouldConnect = true;
			else if(i == 1)
				goToMainMenu();
			break;

		case State::LOBBY_CLIENT:
			if(i == 0)
				m_startGame = true;
			else if(i == 1)
				goToMainMenu();
			break;

		case State::SETTINGS:
			if(i == 0)
			{
				m_menuMusicEnabled = !m_menuMusicEnabled;
				setupSettings();
			}
			else if(i == 1)
			{
				m_gameMusicEnabled = !m_gameMusicEnabled;
				setupSettings();
			}
			else if(i == 2)
			{
				goToMainMenu();
			}
			break;

		case State::MAP_SELECT:
		{
			std::vector<std::string> const maps = {"Test1", "Test2", "Test3", "Test4"};
			if(i < maps.size())
			{
				m_selectedMap = maps[i];
				setupMapSelection();
			}
			else
			{
				goToMainMenu();
			}
			break;
		}

		case State::MODE_SELECT:
		{
			std::vector<std::string> const modes = {"Deathmatch", "TestMode2", "TestMode3", "TestMode4"};
			if(i < modes.size())
			{
				m_selectedMode = modes[i];
				setupModeSelection();
			}
			else
			{
				goToMainMenu();
			}
			break;
		}
		}
		return;
	}
}

void Menu::draw(sf::RenderWindow &window) const
{
	window.clear(sf::Color(30, 30, 30));
	window.draw(m_title);

	if(m_state == State::MAIN)
	{
		window.draw(m_playerNameLabel);
		window.draw(m_playerNameBox);
		window.draw(m_playerNameText);
	}

	if(m_state == State::JOIN_LOBBY)
	{
		window.draw(m_serverIpBox);
		window.draw(m_serverIpText);
		window.draw(m_serverPortBox);
		window.draw(m_serverPortText);
	}

	for(auto const &text : m_lobbyTexts)
	{
		window.draw(text);
	}

	for(size_t i = 0; i < m_buttonShapes.size(); ++i)
	{
		window.draw(m_buttonShapes[i]);
		window.draw(m_buttonTexts[i]);
	}
}

void Menu::updateLobbyDisplay(std::vector<LobbyPlayerInfo> const &players)
{
	if(m_state != State::LOBBY_CLIENT)
		return;

	while(m_lobbyTexts.size() > 1)
		m_lobbyTexts.pop_back();

	// update player count label
	sf::Text playersLabel(m_font);
	playersLabel.setString("Players (" + std::to_string(players.size()) + "/4):");
	playersLabel.setCharacterSize(24);
	playersLabel.setFillColor(sf::Color::White);
	playersLabel.setPosition({200.f, 240.f});
	m_lobbyTexts.push_back(playersLabel);

	for(size_t i = 0; i < players.size(); ++i)
	{
		sf::Text playerText(m_font);
		std::string statusStr = players[i].bReady ? "[Ready]" : "[Not Ready]";
		std::string nameStr = players[i].name;
		if(players[i].name == m_playerName)
			nameStr += " (You)";

		playerText.setString(std::to_string(i + 1) + ". " + statusStr + " " + nameStr);
		playerText.setCharacterSize(20);
		playerText.setFillColor(players[i].bReady ? sf::Color(100, 255, 100) : sf::Color(255, 200, 100));
		playerText.setPosition({220.f, 280.f + i * 30.f});
		m_lobbyTexts.push_back(playerText);
	}

	for(size_t i = players.size(); i < 4; ++i)
	{
		sf::Text playerSlot(m_font);
		playerSlot.setString(std::to_string(i + 1) + ". Waiting...");
		playerSlot.setCharacterSize(20);
		playerSlot.setFillColor(sf::Color(150, 150, 150));
		playerSlot.setPosition({220.f, 280.f + i * 30.f});
		m_lobbyTexts.push_back(playerSlot);
	}
}

void Menu::updateHostButton(bool canStartGame, bool hasEnoughPlayers, bool hostReady)
{
	(void)hasEnoughPlayers;
	if(m_buttonTexts.size() < 3)
		return;

	int const buttonIdx = 2;

	std::string label;
	if(!hostReady)
		label = "Ready";
	else if(!canStartGame)
		label = "Unready";
	else
		label = "Start Game";

	m_buttonTexts[buttonIdx].setString(label);
	m_buttonEnabled[buttonIdx] = true;
	centerTextInButton(m_buttonTexts[buttonIdx], m_buttonShapes[buttonIdx]);

	m_buttonShapes[buttonIdx].setFillColor(sf::Color(80, 80, 80));
	m_buttonTexts[buttonIdx].setFillColor(sf::Color::White);
}

void Menu::updateClientButton(bool clientReady)
{
	if(m_state != State::LOBBY_CLIENT || m_buttonTexts.empty())
		return;

	int const buttonIdx = 0;
	m_buttonTexts[buttonIdx].setString("Ready");
	m_buttonEnabled[buttonIdx] = !clientReady;

	sf::Color fillColor = clientReady ? sf::Color(50, 50, 50) : sf::Color(80, 80, 80);
	sf::Color textColor = clientReady ? sf::Color(120, 120, 120) : sf::Color::White;

	m_buttonShapes[buttonIdx].setFillColor(fillColor);
	m_buttonTexts[buttonIdx].setFillColor(textColor);
	centerTextInButton(m_buttonTexts[buttonIdx], m_buttonShapes[buttonIdx]);
}
