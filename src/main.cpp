#include <SFML/Graphics.hpp>
#include <cmath>

#include "Map.h"
#include "Player.h"
#include "HealthBar.h"

int main()
{
	const int WINDOW_WIDTH = 800;
	const int WINDOW_HEIGHT = 600;

	sf::RenderWindow window(sf::VideoMode({WINDOW_WIDTH, WINDOW_HEIGHT}), "Tank Game", sf::Style::Resize);
	window.setFramerateLimit(60);

	Map map{WINDOW_WIDTH, WINDOW_HEIGHT};

	Player player("BlueTank", sf::Color(90, 170, 255), map, 150.f);
	Player enemy("RedTank", sf::Color(255, 170, 90), map, 120.f);
	enemy.setPosition({90, 90});

	HealthBar healthbar({20.f, 20.f}, {220.f, 28.f});

	player.setHealthCallback([&healthbar](int current, int max) {
		healthbar.setMaxHealth(max);
		healthbar.setHealth(current);
	});

	sf::View worldView = window.getDefaultView();
	sf::View hudView = window.getDefaultView();
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

		bool const w = sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::W);
		bool const s = sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::S);
		bool const a = sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::A);
		bool const d = sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::D);

		sf::Vector2f moveVec{static_cast<float>(d - a), static_cast<float>(s - w)};
		player.movement(moveVec);

#ifdef SFML_DEBUG
		bool const h = sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::H);
		if(h)
			player.heal(1);

		bool const r = sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::R);
		if(r)
			player.revive();
#endif

		float frameDelta = frameClock.restart().asSeconds();
		player.update(frameDelta);
		healthbar.update(frameDelta);

		// Draw world
		window.setView(worldView);
		window.clear(sf::Color::White);

		map.draw(window);
		player.draw(window);
		enemy.draw(window);

		// Draw hud
		window.setView(hudView);
		window.draw(healthbar);

		window.display();
	}

	return 0;
}
