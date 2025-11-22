#include <SFML/Graphics.hpp>
#include "HealthBar.h"
#include "GameWorld.h"
#include <SFML/Audio.hpp>
#include <spdlog/spdlog.h>

#include "Utilities.h"

int main()
{
	char constexpr gameName[] = "Tank Game";
	sf::Vector2u constexpr windowDimensions = {800, 600};
	std::shared_ptr<spdlog::logger> const logger = createConsoleLogger(gameName);

	sf::RenderWindow window(sf::VideoMode(windowDimensions), gameName, sf::Style::Resize);
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

		float const dt = frameClock.restart().asMilliseconds();
		gameWorld.update(dt);
		gameWorld.draw(window);
		window.display();
	}

	return 0;
}
