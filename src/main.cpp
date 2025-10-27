#include <SFML/Graphics.hpp>
#include <cmath>
#include <optional>

#include "HealthBar.h"
#include "GameWorld.h"
#include "Menu.h"

enum class GameState { MENU, PLAYING };

int main()
{
	sf::Vector2u const windowDimensions = {800, 600};

	sf::RenderWindow window(sf::VideoMode(windowDimensions), "Tank Game", sf::Style::Resize);
	window.setFramerateLimit(60);
	window.setMinimumSize(windowDimensions);
	window.setMaximumSize(std::optional(sf::Vector2u{3840, 2160}));

	GameState gameState = GameState::MENU;
	Menu menu{windowDimensions};
	std::optional<GameWorld> gameWorld;
	sf::Clock frameClock;

	while(window.isOpen())
	{
		while(const std::optional event = window.pollEvent())
		{
			if(event->is<sf::Event::Closed>())
			{
				window.close();
			}
			else if(const auto *keyPressed = event->getIf<sf::Event::KeyPressed>())
			{
				if(keyPressed->scancode == sf::Keyboard::Scancode::Escape)
					window.close();
			}
			else if(const auto *mouseButtonPressed = event->getIf<sf::Event::MouseButtonPressed>())
			{
				if(gameState == GameState::MENU && mouseButtonPressed->button == sf::Mouse::Button::Left)
				{
					sf::Vector2f mousePos = window.mapPixelToCoords(mouseButtonPressed->position);
					menu.handleClick(mousePos);
				}
			}
			else if(const auto *mouseMoved = event->getIf<sf::Event::MouseMoved>())
			{
				if(gameState == GameState::MENU)
				{
					sf::Vector2f mousePos = window.mapPixelToCoords(mouseMoved->position);
					menu.handleMouseMove(mousePos);
				}
			}
			else if(const auto *textEntered = event->getIf<sf::Event::TextEntered>())
			{
				if(gameState == GameState::MENU && textEntered->unicode < 128)
				{
					menu.handleTextInput(static_cast<char>(textEntered->unicode));
				}
			}
		}

		if(gameState == GameState::MENU)
		{
			if(menu.shouldStartGame())
			{
				gameWorld.emplace(windowDimensions);
				gameState = GameState::PLAYING;
			}
			else if(menu.shouldExit())
			{
				window.close();
			}
			else
			{
				menu.draw(window);
				window.display();
			}
		}
		else if(gameState == GameState::PLAYING && gameWorld.has_value())
		{
			float const dt = frameClock.restart().asSeconds();
			gameWorld->update(dt);
			gameWorld->draw(window);
			window.display();
		}
	}

	return 0;
}
