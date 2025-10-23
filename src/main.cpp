#include "HealthBar.h"

#include <SFML/Graphics.hpp>
#include <vector>
#include <cmath>

#include "Map.h"
#include "Player.h"

int main()
{
	const int WINDOW_WIDTH = 800;
	const int WINDOW_HEIGHT = 600;

	sf::RenderWindow window(sf::VideoMode({WINDOW_WIDTH, WINDOW_HEIGHT}), "Tank Game", sf::Style::Resize);
	window.setFramerateLimit(60);

	Map map{WINDOW_WIDTH, WINDOW_HEIGHT};

	Player player("BlueTank", sf::Color(80, 180, 255), map, 150.f);

	// HUD setup, TODO move into class
	sf::Font hudFont;
	if(!hudFont.openFromFile("../assets/LiberationSans-Regular.ttf"))
		return -1;
	HealthBar hud(220.f, 28.f, hudFont);
	hud.setFont(hudFont);
	hud.setPositionScreen({20.f, 20.f});
	hud.setMaxHealth(150.f);
	hud.setHealth(150.f);

	player.setHealthCallback([&hud](float current, float max) {
		hud.setMaxHealth(max);
		hud.setHealth(current);
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

		float frameDelta = frameClock.restart().asSeconds();
		player.update(frameDelta);
		hud.update(frameDelta);

		// Draw world
		window.setView(window.getDefaultView());
		window.clear(sf::Color::White);

		map.draw(window);
		player.draw(window);

		// Draw hud
		window.setView(hudView);
		window.draw(hud);

		window.display();
	}

	return 0;
}
