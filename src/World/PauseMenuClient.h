#pragma once
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Audio/Music.hpp>

class PauseMenuClient
{
  public:
	explicit PauseMenuClient(sf::RenderWindow &window);
	void draw(sf::RenderWindow &window) const;
	void handleKeyboardEvent(sf::Event::KeyPressed const &keyEvent);
	void handleClick(sf::Vector2f mousePos);

	bool isPaused() const
	{
		return m_isPaused;
	}
	bool isDisconnectRequested() const
	{
		return m_shouldDisconnect;
	}
	void startBattleMusic()
	{
		m_battleMusic.play();
	}

  private:
	enum class PauseMenuState
	{
		MAIN,
		SETTINGS
	};
	std::string getWindowSizeString() const;
	bool m_isPaused = false;
	bool m_shouldDisconnect = false;
	PauseMenuState m_pauseMenuState = PauseMenuState::MAIN;
	sf::Font &m_pauseFont;
	sf::RenderWindow &m_window;
	std::vector<std::string> m_buttonLabels;
	sf::Music &m_battleMusic;
	size_t m_currentWindowSizeIndex = 0;
};
