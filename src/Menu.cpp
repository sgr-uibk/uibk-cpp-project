#include "Menu.h"
#include "Lobby/LobbyClient.h"
#include "ResourceManager.h"
#include "Networking.h"
#include "GameConfig.h"
#include <spdlog/spdlog.h>
#include <algorithm>

Menu::Menu(sf::Vector2u windowDimensions)
	: m_windowDimensions(windowDimensions)
	, m_font(g_assetPathResolver.resolveRelative("Font/LiberationSans-Regular.ttf").string())
	, m_title(m_font)
	, m_playerNameLabel(m_font)
	, m_playerNameText(m_font)
	, m_state(State::MAIN)
	, m_startGame(false)
	, m_exit(false)
	, m_shouldConnect(false)
	, m_playerName("Player")
	, m_editingName(false)
	, m_menuMusicEnabled(true)
	, m_gameMusicEnabled(true)
	, m_serverIp("127.0.0.1")
	, m_serverPort(PORT_TCP)
	, m_serverIpText(m_font)
	, m_serverPortText(m_font)
	, m_editingField(EditingField::NONE)
	, m_selectedMap("Arena")
	, m_selectedMode("Deathmatch")
{
	m_title.setString("TANK GAME");
	m_title.setCharacterSize(50);
	m_title.setFillColor(sf::Color::White);
	sf::FloatRect titleBounds = m_title.getLocalBounds();
	m_title.setPosition({
		(windowDimensions.x - titleBounds.size.x) / 2.f - titleBounds.position.x,
		80.f
	});

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

	m_title.setString("TANK GAME");
	sf::FloatRect titleBounds = m_title.getLocalBounds();
	m_title.setPosition({
		(m_windowDimensions.x - titleBounds.size.x) / 2.f - titleBounds.position.x,
		80.f
	});

	setupMainMenu();
}

void Menu::setTitle(const std::string& title)
{
	m_title.setString(title);
	sf::FloatRect titleBounds = m_title.getLocalBounds();
	m_title.setPosition({
		(m_windowDimensions.x - titleBounds.size.x) / 2.f - titleBounds.position.x,
		80.f
	});
}

void Menu::setupMainMenu()
{
	m_buttonShapes.clear();
	m_buttonTexts.clear();
	m_buttonEnabled.clear();
	m_lobbyTexts.clear();

	const float buttonWidth = 280.f;
	const float buttonHeight = 45.f;
	const float spacing = 15.f;
	const float startY = 200.f;

	std::vector<std::string> labels = {"Host Lobby", "Join Lobby", "Settings", "Exit"};

	for(size_t i = 0; i < labels.size(); ++i)
	{
		sf::RectangleShape shape;
		shape.setSize({buttonWidth, buttonHeight});
		shape.setPosition({
			(m_windowDimensions.x - buttonWidth) / 2.f,
			startY + i * (buttonHeight + spacing)
		});
		shape.setFillColor(sf::Color(80, 80, 80));

		sf::Text text(m_font);
		text.setString(labels[i]);
		text.setCharacterSize(20);
		text.setFillColor(sf::Color::White);
		sf::FloatRect textBounds = text.getLocalBounds();
		text.setPosition({
			shape.getPosition().x + (buttonWidth - textBounds.size.x) / 2.f - textBounds.position.x,
			shape.getPosition().y + (buttonHeight - textBounds.size.y) / 2.f - textBounds.position.y
		});

		m_buttonShapes.push_back(shape);
		m_buttonTexts.push_back(text);
		m_buttonEnabled.push_back(true);
	}
}

void Menu::setupLobbyHost()
{
	m_buttonShapes.clear();
	m_buttonTexts.clear();
	m_buttonEnabled.clear();
	m_lobbyTexts.clear();

	const float buttonWidth = 200.f;
	const float buttonHeight = 50.f;

	sf::Text lobbyInfo(m_font);
	lobbyInfo.setString("Server running on port " + std::to_string(PORT_TCP));
	lobbyInfo.setCharacterSize(20);
	lobbyInfo.setFillColor(sf::Color(200, 200, 200));
	sf::FloatRect infoBounds = lobbyInfo.getLocalBounds();
	lobbyInfo.setPosition({
		(m_windowDimensions.x - infoBounds.size.x) / 2.f - infoBounds.position.x,
		180.f
	});
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
		playerSlot.setPosition({220.f, 280.f + (i-1) * 30.f});
		m_lobbyTexts.push_back(playerSlot);
	}

	// map selection
	const float smallButtonWidth = 180.f;
	const float smallButtonHeight = 35.f;

	sf::Text mapLabel(m_font);
	mapLabel.setString("Map:");
	mapLabel.setCharacterSize(18);
	mapLabel.setFillColor(sf::Color::White);
	mapLabel.setPosition({50.f, 410.f});
	m_lobbyTexts.push_back(mapLabel);

	sf::RectangleShape mapButton;
	mapButton.setSize({smallButtonWidth, smallButtonHeight});
	mapButton.setPosition({120.f, 405.f});
	mapButton.setFillColor(sf::Color(60, 60, 100));
	m_buttonShapes.push_back(mapButton);

	sf::Text mapText(m_font);
	mapText.setString(m_selectedMap);
	mapText.setCharacterSize(16);
	mapText.setFillColor(sf::Color::White);
	sf::FloatRect mapBounds = mapText.getLocalBounds();
	mapText.setPosition({
		mapButton.getPosition().x + (smallButtonWidth - mapBounds.size.x) / 2.f - mapBounds.position.x,
		mapButton.getPosition().y + (smallButtonHeight - mapBounds.size.y) / 2.f - mapBounds.position.y
	});
	m_buttonTexts.push_back(mapText);
	m_buttonEnabled.push_back(true);

	// mode selection
	sf::Text modeLabel(m_font);
	modeLabel.setString("Mode:");
	modeLabel.setCharacterSize(18);
	modeLabel.setFillColor(sf::Color::White);
	modeLabel.setPosition({320.f, 410.f});
	m_lobbyTexts.push_back(modeLabel);

	sf::RectangleShape modeButton;
	modeButton.setSize({smallButtonWidth, smallButtonHeight});
	modeButton.setPosition({390.f, 405.f});
	modeButton.setFillColor(sf::Color(60, 100, 60));
	m_buttonShapes.push_back(modeButton);

	sf::Text modeText(m_font);
	modeText.setString(m_selectedMode);
	modeText.setCharacterSize(16);
	modeText.setFillColor(sf::Color::White);
	sf::FloatRect modeBounds = modeText.getLocalBounds();
	modeText.setPosition({
		modeButton.getPosition().x + (smallButtonWidth - modeBounds.size.x) / 2.f - modeBounds.position.x,
		modeButton.getPosition().y + (smallButtonHeight - modeBounds.size.y) / 2.f - modeBounds.position.y
	});
	m_buttonTexts.push_back(modeText);
	m_buttonEnabled.push_back(true);

	// start game and back buttons
	sf::RectangleShape startButton;
	startButton.setSize({buttonWidth, buttonHeight});
	startButton.setPosition({(m_windowDimensions.x - buttonWidth) / 2.f - 110.f, 450.f});
	startButton.setFillColor(sf::Color(80, 80, 80));
	m_buttonShapes.push_back(startButton);

	sf::Text startText(m_font);
	startText.setString("Start Game");
	startText.setCharacterSize(24);
	startText.setFillColor(sf::Color::White);
	sf::FloatRect startBounds = startText.getLocalBounds();
	startText.setPosition({
		startButton.getPosition().x + (buttonWidth - startBounds.size.x) / 2.f - startBounds.position.x,
		startButton.getPosition().y + (buttonHeight - startBounds.size.y) / 2.f - startBounds.position.y
	});
	m_buttonTexts.push_back(startText);
	m_buttonEnabled.push_back(true);

	sf::RectangleShape backButton;
	backButton.setSize({buttonWidth, buttonHeight});
	backButton.setPosition({(m_windowDimensions.x - buttonWidth) / 2.f + 110.f, 450.f});
	backButton.setFillColor(sf::Color(80, 80, 80));
	m_buttonShapes.push_back(backButton);

	sf::Text backText(m_font);
	backText.setString("Back");
	backText.setCharacterSize(24);
	backText.setFillColor(sf::Color::White);
	sf::FloatRect backBounds = backText.getLocalBounds();
	backText.setPosition({
		backButton.getPosition().x + (buttonWidth - backBounds.size.x) / 2.f - backBounds.position.x,
		backButton.getPosition().y + (buttonHeight - backBounds.size.y) / 2.f - backBounds.position.y
	});
	m_buttonTexts.push_back(backText);
	m_buttonEnabled.push_back(true);
}

void Menu::setupLobbyClient()
{
	m_buttonShapes.clear();
	m_buttonTexts.clear();
	m_buttonEnabled.clear();
	m_lobbyTexts.clear();

	const float buttonWidth = 200.f;
	const float buttonHeight = 50.f;

	sf::Text lobbyInfo(m_font);
	lobbyInfo.setString("Waiting in lobby...");
	lobbyInfo.setCharacterSize(20);
	lobbyInfo.setFillColor(sf::Color(200, 200, 200));
	sf::FloatRect infoBounds = lobbyInfo.getLocalBounds();
	lobbyInfo.setPosition({
		(m_windowDimensions.x - infoBounds.size.x) / 2.f - infoBounds.position.x,
		180.f
	});
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
		playerSlot.setPosition({220.f, 280.f + (i-1) * 30.f});
		m_lobbyTexts.push_back(playerSlot);
	}

	sf::RectangleShape readyButton;
	readyButton.setSize({buttonWidth, buttonHeight});
	readyButton.setPosition({(m_windowDimensions.x - buttonWidth) / 2.f - 110.f, 450.f});
	readyButton.setFillColor(sf::Color(80, 80, 80));
	m_buttonShapes.push_back(readyButton);

	sf::Text readyText(m_font);
	readyText.setString("Ready");
	readyText.setCharacterSize(24);
	readyText.setFillColor(sf::Color::White);
	sf::FloatRect readyBounds = readyText.getLocalBounds();
	readyText.setPosition({
		readyButton.getPosition().x + (buttonWidth - readyBounds.size.x) / 2.f - readyBounds.position.x,
		readyButton.getPosition().y + (buttonHeight - readyBounds.size.y) / 2.f - readyBounds.position.y
	});
	m_buttonTexts.push_back(readyText);
	m_buttonEnabled.push_back(true);

	sf::RectangleShape backButton;
	backButton.setSize({buttonWidth, buttonHeight});
	backButton.setPosition({(m_windowDimensions.x - buttonWidth) / 2.f + 110.f, 450.f});
	backButton.setFillColor(sf::Color(80, 80, 80));
	m_buttonShapes.push_back(backButton);

	sf::Text backText(m_font);
	backText.setString("Leave");
	backText.setCharacterSize(24);
	backText.setFillColor(sf::Color::White);
	sf::FloatRect backBounds = backText.getLocalBounds();
	backText.setPosition({
		backButton.getPosition().x + (buttonWidth - backBounds.size.x) / 2.f - backBounds.position.x,
		backButton.getPosition().y + (buttonHeight - backBounds.size.y) / 2.f - backBounds.position.y
	});
	m_buttonTexts.push_back(backText);
	m_buttonEnabled.push_back(true);
}

void Menu::setupJoinLobby()
{
	m_buttonShapes.clear();
	m_buttonTexts.clear();
	m_buttonEnabled.clear();
	m_lobbyTexts.clear();

	const float buttonWidth = 200.f;
	const float buttonHeight = 50.f;

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

	sf::RectangleShape connectButton;
	connectButton.setSize({buttonWidth, buttonHeight});
	connectButton.setPosition({(m_windowDimensions.x - buttonWidth) / 2.f - 110.f, 350.f});
	connectButton.setFillColor(sf::Color(80, 80, 80));
	m_buttonShapes.push_back(connectButton);

	sf::Text connectText(m_font);
	connectText.setString("Connect");
	connectText.setCharacterSize(24);
	connectText.setFillColor(sf::Color::White);
	sf::FloatRect connectBounds = connectText.getLocalBounds();
	connectText.setPosition({
		connectButton.getPosition().x + (buttonWidth - connectBounds.size.x) / 2.f - connectBounds.position.x,
		connectButton.getPosition().y + (buttonHeight - connectBounds.size.y) / 2.f - connectBounds.position.y
	});
	m_buttonTexts.push_back(connectText);
	m_buttonEnabled.push_back(true);

	sf::RectangleShape backButton;
	backButton.setSize({buttonWidth, buttonHeight});
	backButton.setPosition({(m_windowDimensions.x - buttonWidth) / 2.f + 110.f, 350.f});
	backButton.setFillColor(sf::Color(80, 80, 80));
	m_buttonShapes.push_back(backButton);

	sf::Text backText(m_font);
	backText.setString("Back");
	backText.setCharacterSize(24);
	backText.setFillColor(sf::Color::White);
	sf::FloatRect backBounds = backText.getLocalBounds();
	backText.setPosition({
		backButton.getPosition().x + (buttonWidth - backBounds.size.x) / 2.f - backBounds.position.x,
		backButton.getPosition().y + (buttonHeight - backBounds.size.y) / 2.f - backBounds.position.y
	});
	m_buttonTexts.push_back(backText);
	m_buttonEnabled.push_back(true);
}

void Menu::setupSettings()
{
	m_buttonShapes.clear();
	m_buttonTexts.clear();
	m_buttonEnabled.clear();
	m_lobbyTexts.clear();

	const float buttonWidth = 280.f;
	const float buttonHeight = 42.f;
	const float startY = 180.f;

	sf::Text audioHeader(m_font);
	audioHeader.setString("AUDIO SETTINGS");
	audioHeader.setCharacterSize(20);
	audioHeader.setFillColor(sf::Color(200, 200, 100));
	audioHeader.setPosition({(m_windowDimensions.x - 180.f) / 2.f, startY});
	m_lobbyTexts.push_back(audioHeader);

	sf::RectangleShape menuMusicButton;
	menuMusicButton.setSize({buttonWidth, buttonHeight});
	menuMusicButton.setPosition({(m_windowDimensions.x - buttonWidth) / 2.f, startY + 35.f});
	menuMusicButton.setFillColor(sf::Color(80, 80, 80));
	m_buttonShapes.push_back(menuMusicButton);

	sf::Text menuMusicText(m_font);
	std::string menuMusicStr = m_menuMusicEnabled ? "Menu Music: ON" : "Menu Music: OFF";
	menuMusicText.setString(menuMusicStr);
	menuMusicText.setCharacterSize(18);
	menuMusicText.setFillColor(sf::Color::White);
	sf::FloatRect menuBounds = menuMusicText.getLocalBounds();
	menuMusicText.setPosition({
		menuMusicButton.getPosition().x + (buttonWidth - menuBounds.size.x) / 2.f - menuBounds.position.x,
		menuMusicButton.getPosition().y + (buttonHeight - menuBounds.size.y) / 2.f - menuBounds.position.y
	});
	m_buttonTexts.push_back(menuMusicText);
	m_buttonEnabled.push_back(true);

	sf::RectangleShape gameMusicButton;
	gameMusicButton.setSize({buttonWidth, buttonHeight});
	gameMusicButton.setPosition({(m_windowDimensions.x - buttonWidth) / 2.f, startY + 85.f});
	gameMusicButton.setFillColor(sf::Color(80, 80, 80));
	m_buttonShapes.push_back(gameMusicButton);

	sf::Text gameMusicText(m_font);
	std::string gameMusicStr = m_gameMusicEnabled ? "Game Music: ON" : "Game Music: OFF";
	gameMusicText.setString(gameMusicStr);
	gameMusicText.setCharacterSize(18);
	gameMusicText.setFillColor(sf::Color::White);
	sf::FloatRect gameBounds = gameMusicText.getLocalBounds();
	gameMusicText.setPosition({
		gameMusicButton.getPosition().x + (buttonWidth - gameBounds.size.x) / 2.f - gameBounds.position.x,
		gameMusicButton.getPosition().y + (buttonHeight - gameBounds.size.y) / 2.f - gameBounds.position.y
	});
	m_buttonTexts.push_back(gameMusicText);
	m_buttonEnabled.push_back(true);

	sf::Text keybindsHeader(m_font);
	keybindsHeader.setString("KEYBINDS");
	keybindsHeader.setCharacterSize(20);
	keybindsHeader.setFillColor(sf::Color(200, 200, 100));
	keybindsHeader.setPosition({(m_windowDimensions.x - 110.f) / 2.f, startY + 145.f});
	m_lobbyTexts.push_back(keybindsHeader);

	std::vector<std::string> keybindLabels = {
		"Move: W A S D",
		"Shoot: Space / Click",
		"Use Item: Q",
		"Hotbar: 1-9 / Wheel",
		"Pause: Escape"
	};

	const float leftColX = 150.f;
	const float rightColX = 450.f;
	const float keybindStartY = startY + 175.f;
	const float keybindSpacing = 24.f;

	for(size_t i = 0; i < keybindLabels.size(); ++i)
	{
		sf::Text keybindText(m_font);
		keybindText.setString(keybindLabels[i]);
		keybindText.setCharacterSize(15);
		keybindText.setFillColor(sf::Color(180, 180, 180));

		if(i < 3)
		{
			keybindText.setPosition({leftColX, keybindStartY + i * keybindSpacing});
		}
		else
		{
			keybindText.setPosition({rightColX, keybindStartY + (i - 3) * keybindSpacing});
		}
		m_lobbyTexts.push_back(keybindText);
	}

	sf::RectangleShape backButton;
	backButton.setSize({buttonWidth, buttonHeight});
	backButton.setPosition({(m_windowDimensions.x - buttonWidth) / 2.f, startY + 255.f});
	backButton.setFillColor(sf::Color(80, 80, 80));
	m_buttonShapes.push_back(backButton);

	sf::Text backText(m_font);
	backText.setString("Back");
	backText.setCharacterSize(20);
	backText.setFillColor(sf::Color::White);
	sf::FloatRect backBounds = backText.getLocalBounds();
	backText.setPosition({
		backButton.getPosition().x + (buttonWidth - backBounds.size.x) / 2.f - backBounds.position.x,
		backButton.getPosition().y + (buttonHeight - backBounds.size.y) / 2.f - backBounds.position.y
	});
	m_buttonTexts.push_back(backText);
	m_buttonEnabled.push_back(true);
}

void Menu::setupMapSelection()
{
	m_buttonShapes.clear();
	m_buttonTexts.clear();
	m_buttonEnabled.clear();
	m_lobbyTexts.clear();

	const float buttonWidth = 250.f;
	const float buttonHeight = 50.f;
	const float startY = 180.f;

	const std::vector<std::string> maps = {"Test1", "Test2", "Test3", "Test4"};

	for(size_t i = 0; i < maps.size(); ++i)
	{
		sf::RectangleShape mapButton;
		mapButton.setSize({buttonWidth, buttonHeight});
		mapButton.setPosition({(m_windowDimensions.x - buttonWidth) / 2.f, startY + i * (buttonHeight + 15.f)});

		if(maps[i] == m_selectedMap)
			mapButton.setFillColor(sf::Color(60, 120, 60));
		else
			mapButton.setFillColor(sf::Color(80, 80, 80));

		m_buttonShapes.push_back(mapButton);

		sf::Text mapText(m_font);
		mapText.setString(maps[i]);
		mapText.setCharacterSize(22);
		mapText.setFillColor(sf::Color::White);
		sf::FloatRect textBounds = mapText.getLocalBounds();
		mapText.setPosition({
			mapButton.getPosition().x + (buttonWidth - textBounds.size.x) / 2.f - textBounds.position.x,
			mapButton.getPosition().y + (buttonHeight - textBounds.size.y) / 2.f - textBounds.position.y
		});
		m_buttonTexts.push_back(mapText);
		m_buttonEnabled.push_back(true);
	}

	sf::RectangleShape backButton;
	backButton.setSize({buttonWidth, buttonHeight});
	backButton.setPosition({(m_windowDimensions.x - buttonWidth) / 2.f, startY + maps.size() * (buttonHeight + 15.f) + 30.f});
	backButton.setFillColor(sf::Color(80, 80, 80));
	m_buttonShapes.push_back(backButton);

	sf::Text backText(m_font);
	backText.setString("Back");
	backText.setCharacterSize(24);
	backText.setFillColor(sf::Color::White);
	sf::FloatRect backBounds = backText.getLocalBounds();
	backText.setPosition({
		backButton.getPosition().x + (buttonWidth - backBounds.size.x) / 2.f - backBounds.position.x,
		backButton.getPosition().y + (buttonHeight - backBounds.size.y) / 2.f - backBounds.position.y
	});
	m_buttonTexts.push_back(backText);
	m_buttonEnabled.push_back(true);
}

void Menu::setupModeSelection()
{
	m_buttonShapes.clear();
	m_buttonTexts.clear();
	m_buttonEnabled.clear();
	m_lobbyTexts.clear();

	const float buttonWidth = 250.f;
	const float buttonHeight = 50.f;
	const float startY = 180.f;

	const std::vector<std::string> modes = {"Deathmatch", "TestMode2", "TestMode3", "TestMode4"};

	for(size_t i = 0; i < modes.size(); ++i)
	{
		sf::RectangleShape modeButton;
		modeButton.setSize({buttonWidth, buttonHeight});
		modeButton.setPosition({(m_windowDimensions.x - buttonWidth) / 2.f, startY + i * (buttonHeight + 15.f)});

		if(modes[i] == m_selectedMode)
			modeButton.setFillColor(sf::Color(60, 120, 60));
		else
			modeButton.setFillColor(sf::Color(80, 80, 80));

		m_buttonShapes.push_back(modeButton);

		sf::Text modeText(m_font);
		modeText.setString(modes[i]);
		modeText.setCharacterSize(22);
		modeText.setFillColor(sf::Color::White);
		sf::FloatRect textBounds = modeText.getLocalBounds();
		modeText.setPosition({
			modeButton.getPosition().x + (buttonWidth - textBounds.size.x) / 2.f - textBounds.position.x,
			modeButton.getPosition().y + (buttonHeight - textBounds.size.y) / 2.f - textBounds.position.y
		});
		m_buttonTexts.push_back(modeText);
		m_buttonEnabled.push_back(true);
	}

	sf::RectangleShape backButton;
	backButton.setSize({buttonWidth, buttonHeight});
	backButton.setPosition({(m_windowDimensions.x - buttonWidth) / 2.f, startY + modes.size() * (buttonHeight + 15.f) + 30.f});
	backButton.setFillColor(sf::Color(80, 80, 80));
	m_buttonShapes.push_back(backButton);

	sf::Text backText(m_font);
	backText.setString("Back");
	backText.setCharacterSize(24);
	backText.setFillColor(sf::Color::White);
	sf::FloatRect backBounds = backText.getLocalBounds();
	backText.setPosition({
		backButton.getPosition().x + (buttonWidth - backBounds.size.x) / 2.f - backBounds.position.x,
		backButton.getPosition().y + (buttonHeight - backBounds.size.y) / 2.f - backBounds.position.y
	});
	m_buttonTexts.push_back(backText);
	m_buttonEnabled.push_back(true);
}

bool Menu::isMouseOver(const sf::RectangleShape& shape, sf::Vector2f mousePos) const
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
		m_playerNameBox.setOutlineColor(m_editingField == EditingField::PLAYER_NAME ? sf::Color(200, 200, 100) : sf::Color(100, 100, 100));
	}

	if(m_state == State::JOIN_LOBBY)
	{
		if(isMouseOver(m_serverIpBox, mousePos))
		{
			m_serverIpBox.setOutlineColor(sf::Color(150, 150, 150));
		}
		else
		{
			m_serverIpBox.setOutlineColor(m_editingField == EditingField::SERVER_IP ? sf::Color(200, 200, 100) : sf::Color(100, 100, 100));
		}

		if(isMouseOver(m_serverPortBox, mousePos))
		{
			m_serverPortBox.setOutlineColor(sf::Color(150, 150, 150));
		}
		else
		{
			m_serverPortBox.setOutlineColor(m_editingField == EditingField::SERVER_PORT ? sf::Color(200, 200, 100) : sf::Color(100, 100, 100));
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
		else if((c >= '0' && c <= '9') || c == '.' )
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
			m_editingField = (m_editingField == EditingField::SERVER_PORT) ? EditingField::NONE : EditingField::SERVER_PORT;
			return;
		}
	}

	for(size_t i = 0; i < m_buttonShapes.size(); ++i)
	{
		if(m_buttonEnabled[i] && isMouseOver(m_buttonShapes[i], mousePos))
		{
			if(m_state == State::MAIN)
			{
				if(i == 0)
				{
					m_state = State::LOBBY_HOST;
					m_title.setString("WAITING FOR PLAYERS");
					sf::FloatRect titleBounds = m_title.getLocalBounds();
					m_title.setPosition({
						(m_windowDimensions.x - titleBounds.size.x) / 2.f - titleBounds.position.x,
						80.f
					});
					setupLobbyHost();
				}
				else if(i == 1)
				{
					spdlog::info("Join Lobby button clicked, player name is: '{}'", m_playerName);
					m_state = State::JOIN_LOBBY;
					m_title.setString("CONNECT TO SERVER");
					sf::FloatRect titleBounds = m_title.getLocalBounds();
					m_title.setPosition({
						(m_windowDimensions.x - titleBounds.size.x) / 2.f - titleBounds.position.x,
						80.f
					});
					setupJoinLobby();
				}
				else if(i == 2)
				{
					m_state = State::SETTINGS;
					m_title.setString("SETTINGS");
					sf::FloatRect titleBounds = m_title.getLocalBounds();
					m_title.setPosition({
						(m_windowDimensions.x - titleBounds.size.x) / 2.f - titleBounds.position.x,
						80.f
					});
					setupSettings();
				}
				else if(i == 3)
				{
					m_exit = true;
				}
			}
			else if(m_state == State::LOBBY_HOST)
			{
				if(i == 0)
				{
					const std::vector<std::string> maps = {"Test1", "Test2", "Test3", "Test4"};
					auto it = std::find(maps.begin(), maps.end(), m_selectedMap);
					if(it != maps.end())
					{
						++it;
						if(it == maps.end()) it = maps.begin();
						m_selectedMap = *it;
					}
					else
					{
						m_selectedMap = maps[0];
					}
					setupLobbyHost();
				}
				else if(i == 1)
				{
					const std::vector<std::string> modes = {"Deathmatch", "TestMode2", "TestMode3", "TestMode4"};
					auto it = std::find(modes.begin(), modes.end(), m_selectedMode);
					if(it != modes.end())
					{
						++it;
						if(it == modes.end()) it = modes.begin();
						m_selectedMode = *it;
					}
					else
					{
						m_selectedMode = modes[0];
					}
					setupLobbyHost();
				}
				else if(i == 2)
				{
					m_startGame = true;
				}
				else if(i == 3)
				{
					m_state = State::MAIN;
					m_title.setString("TANK GAME");
					sf::FloatRect titleBounds = m_title.getLocalBounds();
					m_title.setPosition({
						(m_windowDimensions.x - titleBounds.size.x) / 2.f - titleBounds.position.x,
						80.f
					});
					setupMainMenu();
				}
			}
			else if(m_state == State::JOIN_LOBBY)
			{
				if(i == 0)
				{
					m_shouldConnect = true;
				}
				else if(i == 1)
				{
					m_state = State::MAIN;
					m_title.setString("TANK GAME");
					sf::FloatRect titleBounds = m_title.getLocalBounds();
					m_title.setPosition({
						(m_windowDimensions.x - titleBounds.size.x) / 2.f - titleBounds.position.x,
						80.f
					});
					setupMainMenu();
				}
			}
			else if(m_state == State::LOBBY_CLIENT)
			{
				if(i == 0)
				{
					m_startGame = true;
				}
				else if(i == 1)
				{
					m_state = State::MAIN;
					m_title.setString("TANK GAME");
					sf::FloatRect titleBounds = m_title.getLocalBounds();
					m_title.setPosition({
						(m_windowDimensions.x - titleBounds.size.x) / 2.f - titleBounds.position.x,
						80.f
					});
					setupMainMenu();
				}
			}
			else if(m_state == State::SETTINGS)
			{
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
					m_state = State::MAIN;
					m_title.setString("TANK GAME");
					sf::FloatRect titleBounds = m_title.getLocalBounds();
					m_title.setPosition({
						(m_windowDimensions.x - titleBounds.size.x) / 2.f - titleBounds.position.x,
						80.f
					});
					setupMainMenu();
				}
			}
			else if(m_state == State::MAP_SELECT)
			{
				const std::vector<std::string> maps = {"Test1", "Test2", "Test3", "Test4"};
				if(i < maps.size())
				{
					m_selectedMap = maps[i];
					setupMapSelection();
				}
				else
				{
					m_state = State::MAIN;
					m_title.setString("TANK GAME");
					sf::FloatRect titleBounds = m_title.getLocalBounds();
					m_title.setPosition({
						(m_windowDimensions.x - titleBounds.size.x) / 2.f - titleBounds.position.x,
						80.f
					});
					setupMainMenu();
				}
			}
			else if(m_state == State::MODE_SELECT)
			{
				const std::vector<std::string> modes = {"Deathmatch", "TestMode2", "TestMode3", "TestMode4"};
				if(i < modes.size())
				{
					m_selectedMode = modes[i];
					setupModeSelection();
				}
				else
				{
					m_state = State::MAIN;
					m_title.setString("TANK GAME");
					sf::FloatRect titleBounds = m_title.getLocalBounds();
					m_title.setPosition({
						(m_windowDimensions.x - titleBounds.size.x) / 2.f - titleBounds.position.x,
						80.f
					});
					setupMainMenu();
				}
			}
			return;
		}
	}
}

void Menu::draw(sf::RenderWindow& window) const
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

	for(const auto& text : m_lobbyTexts)
	{
		window.draw(text);
	}

	for(size_t i = 0; i < m_buttonShapes.size(); ++i)
	{
		window.draw(m_buttonShapes[i]);
		window.draw(m_buttonTexts[i]);
	}
}

void Menu::updateLobbyDisplay(const std::vector<LobbyPlayerInfo>& players)
{
	if(m_state != State::LOBBY_HOST && m_state != State::LOBBY_CLIENT)
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

		playerText.setString(std::to_string(i+1) + ". " + statusStr + " " + nameStr);
		playerText.setCharacterSize(20);
		playerText.setFillColor(players[i].bReady ? sf::Color(100, 255, 100) : sf::Color(255, 200, 100));
		playerText.setPosition({220.f, 280.f + i * 30.f});
		m_lobbyTexts.push_back(playerText);
	}

	for(size_t i = players.size(); i < 4; ++i)
	{
		sf::Text playerSlot(m_font);
		playerSlot.setString(std::to_string(i+1) + ". Waiting...");
		playerSlot.setCharacterSize(20);
		playerSlot.setFillColor(sf::Color(150, 150, 150));
		playerSlot.setPosition({220.f, 280.f + i * 30.f});
		m_lobbyTexts.push_back(playerSlot);
	}
}

void Menu::updateHostButton(bool canStartGame, bool hasEnoughPlayers, bool hostReady)
{
	if(m_state != State::LOBBY_HOST || m_buttonTexts.size() < 3)
		return;

	const int buttonIdx = 2;
	const float buttonWidth = 200.f;
	const float buttonHeight = 50.f;

	if(!hostReady)
	{
		m_buttonTexts[buttonIdx].setString("Ready");
		m_buttonEnabled[buttonIdx] = true;
	}
	else if(!canStartGame)
	{
		m_buttonTexts[buttonIdx].setString("Unready");
		m_buttonEnabled[buttonIdx] = true;
	}
	else
	{
		m_buttonTexts[buttonIdx].setString("Start Game");
		m_buttonEnabled[buttonIdx] = true;
	}

	sf::FloatRect textBounds = m_buttonTexts[buttonIdx].getLocalBounds();
	m_buttonTexts[buttonIdx].setPosition({
		m_buttonShapes[buttonIdx].getPosition().x + (buttonWidth - textBounds.size.x) / 2.f - textBounds.position.x,
		m_buttonShapes[buttonIdx].getPosition().y + (buttonHeight - textBounds.size.y) / 2.f - textBounds.position.y
	});

	m_buttonShapes[buttonIdx].setFillColor(m_buttonEnabled[buttonIdx] ? sf::Color(80, 80, 80) : sf::Color(50, 50, 50));
	m_buttonTexts[buttonIdx].setFillColor(m_buttonEnabled[buttonIdx] ? sf::Color::White : sf::Color(120, 120, 120));
}

void Menu::updateClientButton(bool clientReady)
{
	if(m_state != State::LOBBY_CLIENT || m_buttonTexts.size() < 1)
		return;

	// ready button
	const int buttonIdx = 0;
	const float buttonWidth = 200.f;
	const float buttonHeight = 50.f;

	if(!clientReady)
	{
		m_buttonTexts[buttonIdx].setString("Ready");
	}
	else
	{
		m_buttonTexts[buttonIdx].setString("Unready");
	}

	sf::FloatRect textBounds = m_buttonTexts[buttonIdx].getLocalBounds();
	m_buttonTexts[buttonIdx].setPosition({
		m_buttonShapes[buttonIdx].getPosition().x + (buttonWidth - textBounds.size.x) / 2.f - textBounds.position.x,
		m_buttonShapes[buttonIdx].getPosition().y + (buttonHeight - textBounds.size.y) / 2.f - textBounds.position.y
	});
}
