#include "Menu.h"
#include "Lobby/LobbyClient.h"
#include "Utilities.h"
#include "ResourceManager.h"
#include "Networking.h"

#include <spdlog/spdlog.h>

bool isHovering(sf::RectangleShape const &shape, sf::Vector2f mousePos)
{
	return shape.getGlobalBounds().contains(mousePos);
}

Menu::Menu(sf::Vector2u windowDimensions)
	: m_windowDimensions(windowDimensions), m_title(m_font), m_state(State::MAIN), m_textName(m_font), m_textIp(m_font),
	  m_textPort(m_font), m_labelName(m_font), m_labelIp(m_font), m_labelPort(m_font), m_errorText(m_font),
	  m_errorButtonText(m_font), m_serverPort(PORT_TCP)
{
	std::string fontPath = g_assetPathResolver.resolveRelative("Font/LiberationSans-Regular.ttf").string();

	if(!m_font.openFromFile(fontPath))
	{
		spdlog::error("Failed to load font from {}", fontPath);
	}

	m_title.setFont(m_font);
	m_title.setCharacterSize(50);
	m_title.setFillColor(sf::Color::White);

	auto setupInput = [&](sf::RectangleShape &box, sf::Text &txt, sf::Text &lbl, std::string lblStr) {
		box.setSize({200.f, 35.f});
		box.setFillColor(sf::Color(40, 40, 40));
		box.setOutlineColor(sf::Color(100, 100, 100));
		box.setOutlineThickness(2.f);

		txt.setFont(m_font);
		txt.setCharacterSize(18);
		txt.setFillColor(sf::Color::White);

		lbl.setFont(m_font);
		lbl.setString(lblStr);
		lbl.setCharacterSize(18);
		lbl.setFillColor(sf::Color::White);
	};

	setupInput(m_inputBoxName, m_textName, m_labelName, "Player Name:");
	setupInput(m_inputBoxIp, m_textIp, m_labelIp, "Server IP:");
	setupInput(m_inputBoxPort, m_textPort, m_labelPort, "Port:");

	m_textName.setString(m_playerName);
	m_textIp.setString(m_serverIp);
	m_textPort.setString(std::to_string(m_serverPort));

	m_errorOverlay.setFillColor(sf::Color(0, 0, 0, 150));
	m_errorBox.setFillColor(sf::Color(50, 50, 50));
	m_errorBox.setOutlineColor(sf::Color::Red);
	m_errorBox.setOutlineThickness(2.f);
	m_errorButton.setFillColor(sf::Color(80, 80, 80));

	m_errorText.setFont(m_font);
	m_errorButtonText.setFont(m_font);

	setupMainMenu();
}

void Menu::addButton(std::string const &textStr, std::function<void()> callback)
{
	Button btn(m_font);
	btn.onClick = callback;

	btn.shape.setSize({250.f, 50.f});
	btn.shape.setFillColor(sf::Color(80, 80, 80));

	btn.text.setFont(m_font);
	btn.text.setString(textStr);
	btn.text.setCharacterSize(24);
	btn.text.setFillColor(sf::Color::White);

	m_buttons.push_back(btn);
}

void Menu::addLabel(std::string const &textStr, float yPos, int size, sf::Color color)
{
	sf::Text lbl(m_font);
	lbl.setString(textStr);
	lbl.setCharacterSize(size);
	lbl.setFillColor(color);

	sf::FloatRect bounds = lbl.getLocalBounds();
	lbl.setPosition({(m_windowDimensions.x - bounds.size.x) / 2.f, yPos});

	m_labels.push_back(lbl);
}

void Menu::updateLayout()
{
	sf::FloatRect tBounds = m_title.getLocalBounds();
	m_title.setPosition({(m_windowDimensions.x - tBounds.size.x) / 2.f, 80.f});

	float startY = (m_state == State::MAIN) ? 250.f : 350.f;
	float gap = 20.f;

	for(auto &btn : m_buttons)
	{
		sf::Vector2f size = btn.shape.getSize();
		float x = (m_windowDimensions.x - size.x) / 2.f;

		btn.shape.setPosition({x, startY});

		sf::FloatRect txtBounds = btn.text.getLocalBounds();

		float textX = x + (size.x - txtBounds.size.x) / 2.f - txtBounds.position.x;
		float textY = startY + (size.y - txtBounds.size.y) / 2.f - txtBounds.position.y - 5.f;

		btn.text.setPosition({textX, textY});

		startY += size.y + gap;
	}

	float cx = m_windowDimensions.x / 2.f;

	if(m_state == State::MAIN)
	{
		m_labelName.setPosition({cx - 150.f, 180.f});
		m_inputBoxName.setPosition({cx - 30.f, 175.f});
		m_textName.setPosition({cx - 25.f, 180.f});
		m_textName.setString(m_playerName + (m_editingField == EditingField::PLAYER_NAME ? "_" : ""));
	}
	else if(m_state == State::JOIN_LOBBY)
	{
		float yIp = 200.f;
		m_labelIp.setPosition({cx - 150.f, yIp + 5.f});
		m_inputBoxIp.setPosition({cx - 30.f, yIp});
		m_textIp.setPosition({cx - 25.f, yIp + 5.f});
		m_textIp.setString(m_serverIp + (m_editingField == EditingField::SERVER_IP ? "_" : ""));

		float yPort = 250.f;
		m_labelPort.setPosition({cx - 150.f, yPort + 5.f});
		m_inputBoxPort.setSize({100.f, 35.f});
		m_inputBoxPort.setPosition({cx - 30.f, yPort});
		m_textPort.setPosition({cx - 25.f, yPort + 5.f});
		m_textPort.setString(std::to_string(m_serverPort) + (m_editingField == EditingField::SERVER_PORT ? "_" : ""));
	}
}

void Menu::setupMainMenu()
{
	m_state = State::MAIN;
	m_title.setString("TANK GAME");
	m_buttons.clear();
	m_labels.clear();
	m_editingField = EditingField::NONE;

	addButton("Host Game", [this]() { m_hostGame = true; });
	addButton("Join Lobby", [this]() { setupJoinLobby(); });
	addButton("Settings", [this]() { setupSettings(); });
	addButton("Exit", [this]() { m_exit = true; });

	updateLayout();
}

void Menu::setupLobbyHost()
{
	m_state = State::LOBBY_HOST;
	m_title.setString("LOBBY (HOST)");
	m_buttons.clear();
	m_labels.clear();

	addLabel("Server running on port " + std::to_string(PORT_TCP), 160.f, 18, sf::Color(200, 200, 200));
	addLabel("Waiting for players...", 180.f, 16, sf::Color(150, 150, 150));

	addButton("Ready", [this]() { m_startGame = true; });
	addButton("Back", [this]() { setupMainMenu(); });

	updateLayout();
}

void Menu::setupJoinLobby()
{
	m_state = State::JOIN_LOBBY;
	m_title.setString("CONNECT TO SERVER");
	m_buttons.clear();
	m_labels.clear();
	m_editingField = EditingField::NONE;

	addButton("Connect", [this]() { m_shouldConnect = true; });
	addButton("Back", [this]() { setupMainMenu(); });

	updateLayout();
}

void Menu::setupLobbyClient()
{
	m_state = State::LOBBY_CLIENT;
	m_title.setString("IN LOBBY");
	m_buttons.clear();
	m_labels.clear();

	addLabel("Connected to Lobby", 160.f, 18, sf::Color(200, 200, 200));
	addLabel("Waiting for players...", 180.f, 16, sf::Color(150, 150, 150));

	addButton("Ready", [this]() { m_startGame = true; });
	addButton("Disconnect", [this]() { setupMainMenu(); });

	updateLayout();
}

void Menu::setupSettings()
{
	m_state = State::SETTINGS;
	m_title.setString("SETTINGS");
	m_buttons.clear();
	m_labels.clear();

	addButton(std::string("Music: ") + (m_menuMusicEnabled ? "ON" : "OFF"), [this]() {
		m_menuMusicEnabled = !m_menuMusicEnabled;
		setupSettings();
	});

	addLabel("Keybinds:", 280.f, 20, sf::Color::Yellow);
	addLabel("WASD to Move, Click to Shoot", 310.f, 16, sf::Color(200, 200, 200));

	addButton("Back", [this]() { setupMainMenu(); });

	updateLayout();
}

void Menu::handleClick(sf::Vector2f mousePos)
{
	if(m_hasError)
	{
		if(isHovering(m_errorButton, mousePos))
			clearError();
		return;
	}

	for(auto &btn : m_buttons)
	{
		if(btn.enabled && isHovering(btn.shape, mousePos))
		{
			if(btn.onClick)
				btn.onClick();
			return;
		}
	}

	if(m_state == State::MAIN)
	{
		if(isHovering(m_inputBoxName, mousePos))
			m_editingField = EditingField::PLAYER_NAME;
		else
			m_editingField = EditingField::NONE;
	}
	else if(m_state == State::JOIN_LOBBY)
	{
		if(isHovering(m_inputBoxIp, mousePos))
			m_editingField = EditingField::SERVER_IP;
		else if(isHovering(m_inputBoxPort, mousePos))
			m_editingField = EditingField::SERVER_PORT;
		else
			m_editingField = EditingField::NONE;
	}

	updateLayout();
}

void Menu::handleMouseMove(sf::Vector2f mousePos)
{
	for(auto &btn : m_buttons)
	{
		if(isHovering(btn.shape, mousePos))
			btn.shape.setFillColor(sf::Color(100, 100, 100));
		else
			btn.shape.setFillColor(sf::Color(80, 80, 80));
	}

	auto highlight = [&](sf::RectangleShape &box) {
		if(isHovering(box, mousePos))
			box.setOutlineColor(sf::Color::White);
		else
			box.setOutlineColor(sf::Color(100, 100, 100));
	};

	if(m_state == State::MAIN)
		highlight(m_inputBoxName);
	if(m_state == State::JOIN_LOBBY)
	{
		highlight(m_inputBoxIp);
		highlight(m_inputBoxPort);
	}
}

void Menu::handleTextInput(char c)
{
	if(m_editingField == EditingField::NONE)
		return;

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
	}
	else if(m_editingField == EditingField::SERVER_PORT)
	{
		std::string currentPort = std::to_string(m_serverPort);
		if(m_serverPort == 0)
			currentPort = "";

		if(c == '\b')
		{
			if(!currentPort.empty())
				currentPort.pop_back();
		}
		else if(isdigit(c) && currentPort.length() < 5)
		{
			currentPort += c;
		}

		try
		{
			m_serverPort = currentPort.empty() ? 0 : std::stoi(currentPort);
		}
		catch(...)
		{
			m_serverPort = 0;
		}
	}
	updateLayout();
}

void Menu::draw(sf::RenderWindow &window) const
{
	window.clear(sf::Color(30, 30, 30));
	window.draw(m_title);

	for(auto const &btn : m_buttons)
	{
		window.draw(btn.shape);
		window.draw(btn.text);
	}

	for(auto const &lbl : m_labels)
	{
		window.draw(lbl);
	}

	if(m_state == State::MAIN)
	{
		window.draw(m_labelName);
		window.draw(m_inputBoxName);
		window.draw(m_textName);
	}
	else if(m_state == State::JOIN_LOBBY)
	{
		window.draw(m_labelIp);
		window.draw(m_inputBoxIp);
		window.draw(m_textIp);
		window.draw(m_labelPort);
		window.draw(m_inputBoxPort);
		window.draw(m_textPort);
	}

	if(m_hasError)
		drawErrorPopup(window);
}

void Menu::updateLobbyDisplay(std::vector<LobbyPlayerInfo> const &players)
{
	std::erase_if(m_labels, [](sf::Text const &t) {
		std::string s = t.getString();
		return !s.empty() && isdigit(s[0]);
	});

	float startY = 220.f;
	for(size_t i = 0; i < 4; ++i)
	{
		std::string s = std::to_string(i + 1) + ". ";
		sf::Color c = sf::Color(100, 100, 100);

		if(i < players.size())
		{
			s += players[i].name;
			if(players[i].bReady)
			{
				s += " [READY]";
				c = sf::Color::Green;
			}
			else
			{
				c = sf::Color(255, 200, 0);
			}
		}
		else
		{
			s += "Waiting...";
		}
		addLabel(s, startY + (i * 30.f), 20, c);
	}
}

void Menu::disableReadyButton()
{
	if((m_state == State::LOBBY_CLIENT || m_state == State::LOBBY_HOST) && !m_buttons.empty())
	{
		m_buttons[0].enabled = false;
		m_buttons[0].shape.setFillColor(sf::Color(50, 50, 50));
		m_buttons[0].text.setString("Ready");
		m_buttons[0].text.setFillColor(sf::Color(100, 100, 100));
	}
}

void Menu::enableReadyButton()
{
	if((m_state == State::LOBBY_CLIENT || m_state == State::LOBBY_HOST) && !m_buttons.empty())
	{
		m_buttons[0].enabled = true;
		m_buttons[0].shape.setFillColor(sf::Color(100, 100, 100));
		m_buttons[0].text.setString("Ready");
		m_buttons[0].text.setFillColor(sf::Color::White);
	}
}

bool Menu::shouldHostGame()
{
	bool b = m_hostGame;
	m_hostGame = false;
	return b;
}
bool Menu::shouldConnect()
{
	bool b = m_shouldConnect;
	m_shouldConnect = false;
	return b;
}
bool Menu::shouldStartGame()
{
	bool b = m_startGame;
	m_startGame = false;
	return b;
}

void Menu::setTitle(std::string const &title)
{
	m_title.setString(title);
	updateLayout();
}
void Menu::setState(State s)
{
	m_state = s;
}
void Menu::reset()
{
	setupMainMenu();
}

void Menu::showError(std::string const &errorMessage)
{
	m_hasError = true;
	m_errorMessage = errorMessage;
	m_errorOverlay.setSize(sf::Vector2f(m_windowDimensions));

	sf::Vector2f boxSize(400.f, 200.f);
	m_errorBox.setSize(boxSize);
	m_errorBox.setPosition({(m_windowDimensions.x - boxSize.x) / 2.f, (m_windowDimensions.y - boxSize.y) / 2.f});

	m_errorText.setString(errorMessage);
	sf::FloatRect tb = m_errorText.getLocalBounds();
	m_errorText.setPosition({m_errorBox.getPosition().x + (boxSize.x - tb.size.x) / 2.f - tb.position.x,
	                         m_errorBox.getPosition().y + 50.f});

	m_errorButton.setSize({100.f, 40.f});
	m_errorButton.setPosition(
		{m_errorBox.getPosition().x + (boxSize.x - 100.f) / 2.f, m_errorBox.getPosition().y + 130.f});

	m_errorButtonText.setString("OK");
	sf::FloatRect bb = m_errorButtonText.getLocalBounds();
	m_errorButtonText.setPosition({m_errorButton.getPosition().x + (100.f - bb.size.x) / 2.f - bb.position.x,
	                               m_errorButton.getPosition().y + 5.f});
}

void Menu::clearError()
{
	m_hasError = false;
}

void Menu::drawErrorPopup(sf::RenderWindow &window) const
{
	window.draw(m_errorOverlay);
	window.draw(m_errorBox);
	window.draw(m_errorText);
	window.draw(m_errorButton);
	window.draw(m_errorButtonText);
}