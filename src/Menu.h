#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>

struct LobbyPlayerInfo;

class Menu
{
  public:
	enum class State
	{
		MAIN,
		LOBBY_CLIENT,
		JOIN_LOBBY,
		SETTINGS,
		MAP_SELECT,
		MODE_SELECT
	};

	Menu(sf::Vector2u windowDimensions);

	void handleClick(sf::Vector2f mousePos);
	void handleMouseMove(sf::Vector2f mousePos);
	void handleTextInput(char c);
	void draw(sf::RenderWindow &window) const;

	State getState() const
	{
		return m_state;
	}
	bool shouldStartGame() const
	{
		return m_startGame;
	}
	bool shouldExit() const
	{
		return m_exit;
	}
	bool shouldConnect() const
	{
		return m_shouldConnect;
	}
	std::string getServerIp() const
	{
		return m_serverIp;
	}
	uint16_t getServerPort() const
	{
		return m_serverPort;
	}
	std::string getPlayerName() const
	{
		return m_playerName;
	}
	bool isMenuMusicEnabled() const
	{
		return m_menuMusicEnabled;
	}
	bool isGameMusicEnabled() const
	{
		return m_gameMusicEnabled;
	}
	std::string getSelectedMap() const
	{
		return m_selectedMap;
	}
	std::string getSelectedMode() const
	{
		return m_selectedMode;
	}
	void clearConnectFlag()
	{
		m_shouldConnect = false;
	}
	void clearStartGameFlag()
	{
		m_startGame = false;
	}
	void reset();
	void updateLobbyDisplay(std::vector<LobbyPlayerInfo> const &players);
	void setState(State state)
	{
		m_state = state;
	}
	void setTitle(std::string const &title);
	void setupLobbyClient();
	void updateHostButton(bool canStartGame, bool hasEnoughPlayers, bool hostReady);
	void updateClientButton(bool clientReady);

  private:
	sf::Vector2u m_windowDimensions;
	sf::Font m_font;
	sf::Text m_title;

	std::vector<sf::RectangleShape> m_buttonShapes;
	std::vector<sf::Text> m_buttonTexts;
	std::vector<bool> m_buttonEnabled;

	std::vector<sf::Text> m_lobbyTexts;

	sf::Text m_playerNameLabel;
	sf::Text m_playerNameText;
	sf::RectangleShape m_playerNameBox;

	State m_state;
	bool m_startGame;
	bool m_exit;
	bool m_shouldConnect;
	std::string m_playerName;

	bool m_menuMusicEnabled;
	bool m_gameMusicEnabled;

	std::string m_serverIp;
	uint16_t m_serverPort;
	sf::RectangleShape m_serverIpBox;
	sf::RectangleShape m_serverPortBox;
	sf::Text m_serverIpText;
	sf::Text m_serverPortText;
	enum class EditingField
	{
		NONE,
		PLAYER_NAME,
		SERVER_IP,
		SERVER_PORT
	};
	EditingField m_editingField;

	std::string m_selectedMap;
	std::string m_selectedMode;

	void setupMainMenu();
	void setupLobbyHost();
	void setupJoinLobby();
	void setupSettings();
	void setupMapSelection();
	void setupModeSelection();
	void updatePlayerNameDisplay();
	bool isMouseOver(sf::RectangleShape const &shape, sf::Vector2f mousePos) const;

	void clearMenuState();
	void centerTextInButton(sf::Text &text, sf::RectangleShape const &button);
	void updateInputBoxBorder(sf::RectangleShape &box, bool hovered, bool editing);
	void addButton(sf::Vector2f position, sf::Vector2f size, std::string const &label, unsigned int fontSize,
	               sf::Color fillColor = sf::Color(80, 80, 80));
	void goToMainMenu();
};
