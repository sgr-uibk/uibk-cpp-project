#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>

class Menu {
public:
	enum class State { MAIN, LOBBY_HOST, JOIN_LOBBY };

	Menu(sf::Vector2u windowDimensions);

	void handleClick(sf::Vector2f mousePos);
	void handleMouseMove(sf::Vector2f mousePos);
	void handleTextInput(char c);
	void draw(sf::RenderWindow& window) const;

	State getState() const { return m_state; }
	bool shouldStartGame() const { return m_startGame; }
	bool shouldExit() const { return m_exit; }
	void reset() { m_startGame = false; m_exit = false; m_state = State::MAIN; }

private:
	sf::Vector2u m_windowDimensions;
	sf::Font m_font;
	sf::Text m_title;
	sf::Text m_soundToggle;
	sf::RectangleShape m_soundButton;

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
	bool m_soundOn;
	std::string m_playerName;
	bool m_editingName;

	void setupMainMenu();
	void setupLobbyHost();
	void setupJoinLobby();
	void updatePlayerNameDisplay();
	bool isMouseOver(const sf::RectangleShape& shape, sf::Vector2f mousePos) const;
};
