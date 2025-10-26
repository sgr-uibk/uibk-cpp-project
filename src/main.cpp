#include <SFML/Graphics.hpp>
#include <cmath>

#include "HealthBar.h"
#include "GameWorld.h"

int main()
{
	sf::Vector2u const windowDimensions = {800, 600};

	sf::RenderWindow window(sf::VideoMode(windowDimensions), "Tank Game", sf::Style::Resize);
	window.setFramerateLimit(60);
	window.setMinimumSize(windowDimensions);
	window.setMaximumSize(std::optional(sf::Vector2u{3840, 2160}));

	GameWorld gameWorld{windowDimensions};
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
		}

		float const dt = frameClock.restart().asSeconds();
		gameWorld.update(dt);
		gameWorld.draw(window);
		window.display();
	}

	return 0;
}
