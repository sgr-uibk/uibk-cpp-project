#include "Menu.h"

Menu::Menu(sf::Vector2u windowDimensions)
	: m_windowDimensions(windowDimensions)
	, m_font("../assets/LiberationSans-Regular.ttf")
	, m_title(m_font)
	, m_soundToggle(m_font)
	, m_playerNameLabel(m_font)
	, m_playerNameText(m_font)
	, m_state(State::MAIN)
	, m_startGame(false)
	, m_exit(false)
	, m_soundOn(true)
	, m_playerName("Player")
	, m_editingName(false)
{
	m_title.setString("TANK GAME");
	m_title.setCharacterSize(50);
	m_title.setFillColor(sf::Color::White);
	sf::FloatRect titleBounds = m_title.getLocalBounds();
	m_title.setPosition({
		(windowDimensions.x - titleBounds.size.x) / 2.f - titleBounds.position.x,
		80.f
	});

	m_soundButton.setSize({115.f, 40.f});
	m_soundButton.setPosition({windowDimensions.x - 130.f, 20.f});
	m_soundButton.setFillColor(sf::Color(60, 60, 60));

	m_soundToggle.setCharacterSize(18);
	m_soundToggle.setFillColor(sf::Color::White);
	m_soundToggle.setString("Sound: ON");
	m_soundToggle.setPosition({windowDimensions.x - 120.f, 30.f});

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
	if(m_editingName)
		displayText += "_";
	m_playerNameText.setString(displayText);
}

void Menu::setupMainMenu()
{
	m_buttonShapes.clear();
	m_buttonTexts.clear();
	m_buttonEnabled.clear();
	m_lobbyTexts.clear();

	const float buttonWidth = 300.f;
	const float buttonHeight = 50.f;
	const float spacing = 20.f;
	const float startY = 250.f;

	std::vector<std::string> labels = {"Host Lobby", "Join Lobby", "Exit"};

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
		text.setCharacterSize(24);
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
	lobbyInfo.setString("Server running on port 25101");
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
	ipLabel.setPosition({250.f, 220.f});
	m_lobbyTexts.push_back(ipLabel);

	sf::Text ipValue(m_font);
	ipValue.setString("127.0.0.1");
	ipValue.setCharacterSize(20);
	ipValue.setFillColor(sf::Color(200, 200, 200));
	ipValue.setPosition({380.f, 220.f});
	m_lobbyTexts.push_back(ipValue);

	sf::Text portLabel(m_font);
	portLabel.setString("Port:");
	portLabel.setCharacterSize(20);
	portLabel.setFillColor(sf::Color::White);
	portLabel.setPosition({250.f, 270.f});
	m_lobbyTexts.push_back(portLabel);

	sf::Text portValue(m_font);
	portValue.setString("25101");
	portValue.setCharacterSize(20);
	portValue.setFillColor(sf::Color(200, 200, 200));
	portValue.setPosition({380.f, 270.f});
	m_lobbyTexts.push_back(portValue);

	sf::Text note(m_font);
	note.setString("Not implemented yet");
	note.setCharacterSize(16);
	note.setFillColor(sf::Color(150, 150, 150));
	sf::FloatRect noteBounds = note.getLocalBounds();
	note.setPosition({
		(m_windowDimensions.x - noteBounds.size.x) / 2.f - noteBounds.position.x,
		330.f
	});
	m_lobbyTexts.push_back(note);

	sf::RectangleShape connectButton;
	connectButton.setSize({buttonWidth, buttonHeight});
	connectButton.setPosition({(m_windowDimensions.x - buttonWidth) / 2.f - 110.f, 400.f});
	connectButton.setFillColor(sf::Color(50, 50, 50));
	m_buttonShapes.push_back(connectButton);

	sf::Text connectText(m_font);
	connectText.setString("Connect");
	connectText.setCharacterSize(24);
	connectText.setFillColor(sf::Color(120, 120, 120));
	sf::FloatRect connectBounds = connectText.getLocalBounds();
	connectText.setPosition({
		connectButton.getPosition().x + (buttonWidth - connectBounds.size.x) / 2.f - connectBounds.position.x,
		connectButton.getPosition().y + (buttonHeight - connectBounds.size.y) / 2.f - connectBounds.position.y
	});
	m_buttonTexts.push_back(connectText);
	m_buttonEnabled.push_back(false);

	sf::RectangleShape backButton;
	backButton.setSize({buttonWidth, buttonHeight});
	backButton.setPosition({(m_windowDimensions.x - buttonWidth) / 2.f + 110.f, 400.f});
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

	if(isMouseOver(m_soundButton, mousePos))
	{
		m_soundButton.setFillColor(sf::Color(80, 80, 80));
	}
	else
	{
		m_soundButton.setFillColor(sf::Color(60, 60, 60));
	}

	if(m_state == State::MAIN && isMouseOver(m_playerNameBox, mousePos))
	{
		m_playerNameBox.setOutlineColor(sf::Color(150, 150, 150));
	}
	else if(m_state == State::MAIN)
	{
		m_playerNameBox.setOutlineColor(m_editingName ? sf::Color(200, 200, 100) : sf::Color(100, 100, 100));
	}
}

void Menu::handleTextInput(char c)
{
	if(!m_editingName)
		return;

	if(c == '\b')	// If backspace pressed
	{
		if(!m_playerName.empty())
			m_playerName.pop_back();
	}
	else if(c >= ' ' && c <= '~' && m_playerName.length() < 12)  // take only standart english characters
	{
		m_playerName += c;
	}

	updatePlayerNameDisplay();
}

void Menu::handleClick(sf::Vector2f mousePos)
{
	if(isMouseOver(m_soundButton, mousePos))
	{
		m_soundOn = !m_soundOn;
		m_soundToggle.setString(m_soundOn ? "Sound: ON" : "Sound: OFF");
		return;
	}

	if(m_state == State::MAIN && isMouseOver(m_playerNameBox, mousePos))
	{
		m_editingName = !m_editingName;
		updatePlayerNameDisplay();
		return;
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
					m_exit = true;
				}
			}
			else if(m_state == State::LOBBY_HOST)
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
			else if(m_state == State::JOIN_LOBBY)
			{
				if(i == 1)
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
	window.draw(m_soundButton);
	window.draw(m_soundToggle);

	if(m_state == State::MAIN)
	{
		window.draw(m_playerNameLabel);
		window.draw(m_playerNameBox);
		window.draw(m_playerNameText);
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
