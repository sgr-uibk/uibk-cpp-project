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
		LOBBY_HOST,
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
	void handleResize(sf::Vector2u newSize);

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
	void updateLobbyDisplay(const std::vector<LobbyPlayerInfo> &players);
	void setState(State state)
	{
		m_state = state;
	}
	void setTitle(const std::string &title);
	void setupLobbyClient();
	void updateHostButton(bool canStartGame, bool hasEnoughPlayers, bool hostReady);
	void updateClientButton(bool clientReady);
	void showError(const std::string &errorMessage);
	void clearError();
	bool hasError() const
	{
		return m_hasError;
	}

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
	bool m_editingName;

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

	bool m_hasError;
	std::string m_errorMessage;
	sf::RectangleShape m_errorOverlay;
	sf::RectangleShape m_errorBox;
	sf::Text m_errorText;
	sf::RectangleShape m_errorButton;
	sf::Text m_errorButtonText;

	void setupMainMenu();
	void setupLobbyHost();
	void setupJoinLobby();
	void setupSettings();
	void setupMapSelection();
	void setupModeSelection();
	void updatePlayerNameDisplay();
	bool isMouseOver(const sf::RectangleShape &shape, sf::Vector2f mousePos) const;
	void drawErrorPopup(sf::RenderWindow &window) const;
};
