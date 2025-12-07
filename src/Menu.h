#pragma once
#include <SFML/Graphics.hpp>
#include <functional>
#include <vector>
#include <string>

class Menu
{
  public:
	enum class State
	{
		MAIN,
		LOBBY_HOST,
		JOIN_LOBBY,
		LOBBY_CLIENT,
		SETTINGS
	};
	enum class EditingField
	{
		NONE,
		PLAYER_NAME,
		SERVER_IP,
		SERVER_PORT
	};

	Menu(sf::Vector2u windowDimensions);

	void draw(sf::RenderWindow &window) const;
	void handleClick(sf::Vector2f mousePos);
	void handleMouseMove(sf::Vector2f mousePos);
	void handleTextInput(char c);

	bool shouldExit() const
	{
		return m_exit;
	}
	bool shouldHostGame();
	bool shouldConnect();
	bool shouldStartGame();

	std::string getPlayerName() const
	{
		return m_playerName;
	}
	std::string getServerIp() const
	{
		return m_serverIp;
	}
	uint16_t getServerPort() const
	{
		return m_serverPort;
	}
	State getState() const
	{
		return m_state;
	}

	void setState(State s);
	void setTitle(std::string const &title);
	void reset();

	void setupMainMenu();
	void setupLobbyHost();
	void setupLobbyClient();
	void setupJoinLobby();
	void setupSettings();

	void updateLobbyDisplay(std::vector<struct LobbyPlayerInfo> const &players);
	void disableReadyButton();
	void enableReadyButton();

	void showError(std::string const &errorMessage);
	void clearError();
	bool isMenuMusicEnabled() const
	{
		return m_menuMusicEnabled;
	}

  private:
	struct Button
	{
		sf::RectangleShape shape;
		sf::Text text;
		std::function<void()> onClick;
		bool enabled = true;

		Button(sf::Font const &font) : text(font)
		{
		}
	};

	void addButton(std::string const &textStr, std::function<void()> onClick);
	void addLabel(std::string const &textStr, float yPos, int size = 20, sf::Color color = sf::Color::White);
	void updateLayout();
	void drawErrorPopup(sf::RenderWindow &window) const;

	sf::Vector2u m_windowDimensions;
	sf::Font m_font;
	sf::Text m_title;
	State m_state;

	std::vector<Button> m_buttons;
	std::vector<sf::Text> m_labels;

	sf::RectangleShape m_inputBoxName, m_inputBoxIp, m_inputBoxPort;
	sf::Text m_textName, m_textIp, m_textPort;
	sf::Text m_labelName, m_labelIp, m_labelPort;
	EditingField m_editingField = EditingField::NONE;

	bool m_exit = false;
	bool m_hostGame = false;
	bool m_shouldConnect = false;
	bool m_startGame = false;

	std::string m_playerName = "Player";
	std::string m_serverIp = "127.0.0.1";
	std::string m_selectedMap = "Arena";
	std::string m_selectedMode = "Deathmatch";
	bool m_menuMusicEnabled = false;
	bool m_gameMusicEnabled = false;

	bool m_hasError = false;
	std::string m_errorMessage;
	mutable sf::RectangleShape m_errorOverlay, m_errorBox, m_errorButton;
	mutable sf::Text m_errorText, m_errorButtonText;
	uint16_t m_serverPort = 25106;
};