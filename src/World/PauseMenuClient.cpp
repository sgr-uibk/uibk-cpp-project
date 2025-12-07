#include "PauseMenuClient.h"
#include "GameConfig.h"
#include "ResourceManager.h"
#include "Utilities.h"
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Text.hpp>
#include <spdlog/spdlog.h>

PauseMenuClient::PauseMenuClient(sf::RenderWindow &window)
	: m_pauseFont(FontManager::inst().load("Font/LiberationSans-Regular.ttf")), m_window(window),
	  m_battleMusic(initMusic("audio/battle_loop.ogg"))
{
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

void PauseMenuClient::draw(sf::RenderWindow &window) const
{
	if(!m_isPaused)
		return;

	sf::RectangleShape overlay(sf::Vector2f(window.getSize()));
	overlay.setFillColor(sf::Color(0, 0, 0, 180));
	window.draw(overlay);

	if(m_pauseMenuState == PauseMenuState::MAIN)
	{
		sf::Text pauseTitle(m_pauseFont);
		pauseTitle.setString(GameConfig::UI::Text::PAUSE_TITLE);
		pauseTitle.setCharacterSize(48);
		pauseTitle.setFillColor(sf::Color::White);
		sf::FloatRect titleBounds = pauseTitle.getLocalBounds();
		pauseTitle.setPosition(sf::Vector2f(window.getSize().x / 2.f - titleBounds.size.x / 2.f, 80.f));
		window.draw(pauseTitle);

		sf::Text warningText(m_pauseFont);
		warningText.setString(GameConfig::UI::Text::PAUSE_WARNING);
		warningText.setCharacterSize(20);
		warningText.setFillColor(sf::Color(255, 100, 100));
		sf::FloatRect warningBounds = warningText.getLocalBounds();
		warningText.setPosition(sf::Vector2f(window.getSize().x / 2.f - warningBounds.size.x / 2.f, 160.f));
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
		settingsTitle.setPosition(sf::Vector2f(window.getSize().x / 2.f - titleBounds.size.x / 2.f, 60.f));
		window.draw(settingsTitle);

		sf::RectangleShape musicButton;
		musicButton.setSize({GameConfig::UI::MUSIC_BUTTON_WIDTH, GameConfig::UI::MUSIC_BUTTON_HEIGHT});
		musicButton.setPosition({window.getSize().x / 2.f - GameConfig::UI::MUSIC_BUTTON_WIDTH / 2.f, 120.f});
		musicButton.setFillColor(sf::Color(80, 80, 80));
		window.draw(musicButton);

		sf::Text musicText(m_pauseFont);
		bool musicPlaying = m_battleMusic.getStatus() == sf::Music::Status::Playing;
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

		sf::Text keybindsHeader(m_pauseFont);
		keybindsHeader.setString(GameConfig::UI::Text::KEYBINDS_HEADER);
		keybindsHeader.setCharacterSize(28);
		keybindsHeader.setFillColor(sf::Color(200, 200, 100));
		sf::FloatRect headerBounds = keybindsHeader.getLocalBounds();
		keybindsHeader.setPosition(sf::Vector2f(window.getSize().x / 2.f - headerBounds.size.x / 2.f, 190.f));
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
			keybindText.setPosition(sf::Vector2f(window.getSize().x / 2.f - textBounds.size.x / 2.f, 235.f + i * 32.f));
			window.draw(keybindText);
		}

		sf::RectangleShape backButton;
		backButton.setSize({GameConfig::UI::BUTTON_WIDTH, GameConfig::UI::BUTTON_HEIGHT});
		backButton.setPosition(
			{window.getSize().x / 2.f - GameConfig::UI::BUTTON_WIDTH / 2.f, window.getSize().y - 120.f});
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
void PauseMenuClient::handleKeyboardEvent(sf::Event::KeyPressed const &keyEvent)
{
	if(keyEvent.scancode == sf::Keyboard::Scancode::Escape)
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
}

void PauseMenuClient::handleClick(sf::Vector2f mousePos)
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
		sf::FloatRect musicButtonBounds(
			sf::Vector2f(m_window.getSize().x / 2.f - GameConfig::UI::MUSIC_BUTTON_WIDTH / 2.f, 120.f),
			sf::Vector2f(GameConfig::UI::MUSIC_BUTTON_WIDTH, GameConfig::UI::MUSIC_BUTTON_HEIGHT));

		if(musicButtonBounds.contains(mousePos))
		{
			if(m_battleMusic.getStatus() == sf::Music::Status::Playing)
			{
				m_battleMusic.pause();
				spdlog::info("Battle music paused");
			}
			else
			{
				m_battleMusic.play();
				spdlog::info("Battle music started");
			}
			return;
		}

		// if back button clicked
		sf::FloatRect backButtonBounds(
			sf::Vector2f(m_window.getSize().x / 2.f - GameConfig::UI::BUTTON_WIDTH / 2.f, m_window.getSize().y - 120.f),
			sf::Vector2f(GameConfig::UI::BUTTON_WIDTH, GameConfig::UI::BUTTON_HEIGHT));

		if(backButtonBounds.contains(mousePos))
		{
			m_pauseMenuState = PauseMenuState::MAIN;
			spdlog::info("Returned to main pause menu");
		}
	}
}
