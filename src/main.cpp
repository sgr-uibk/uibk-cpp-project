#include "HealthBar.h"

#include <SFML/Graphics.hpp>
#include <vector>
#include <cmath>

int main()
{
	const int WINDOW_WIDTH = 800;
	const int WINDOW_HEIGHT = 600;
	const float WALL_THICKNESS = 20.f;

	sf::RenderWindow window(sf::VideoMode({WINDOW_WIDTH, WINDOW_HEIGHT}), "Tank Game", sf::Style::Resize);
	window.setFramerateLimit(60);

	sf::Texture tankTexture;
	if(!tankTexture.loadFromFile("../assets/tank.png"))
		return -1;
	tankTexture.setSmooth(true);

	sf::Sprite tankSprite(tankTexture);
	tankSprite.setOrigin({tankTexture.getSize().x / 2.f, tankTexture.getSize().y / 2.f});
	constexpr float tankW = 64.f, tankH = 64.f;
	tankSprite.setScale({tankW / tankTexture.getSize().x, tankH / tankTexture.getSize().y});
	tankSprite.setPosition({368.f + tankW / 2.f, 268.f + tankH / 2.f});

	std::vector<sf::RectangleShape> walls;
	auto addWall = [&](float x, float y, float w, float h) {
		sf::RectangleShape r({w, h});
		r.setPosition({x, y});
		r.setFillColor(sf::Color::Black);
		walls.push_back(r);
	};

	// Outer and inner walls
	addWall(0, 0, (float)WINDOW_WIDTH, WALL_THICKNESS);
	addWall(0, WINDOW_HEIGHT - WALL_THICKNESS, (float)WINDOW_WIDTH, WALL_THICKNESS);
	addWall(0, 0, WALL_THICKNESS, (float)WINDOW_HEIGHT);
	addWall(WINDOW_WIDTH - WALL_THICKNESS, 0, WALL_THICKNESS, (float)WINDOW_HEIGHT);

	addWall(200, 100, WALL_THICKNESS, 400); // vertical
	addWall(400, 200, 200, WALL_THICKNESS); // horizontal
	addWall(600, 50, WALL_THICKNESS, 300);  // vertical

	// HUD setup
	sf::Font hudFont;
	if(!hudFont.openFromFile("../assets/LiberationSans-Regular.ttf"))
	{
		return -1;
	}
	HealthBar hud(220.f, 28.f, hudFont);
	hud.setFont(hudFont);
	hud.setPositionScreen({20.f, 20.f});
	hud.setMaxHealth(150.f);
	hud.setHealth(150.f);

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

		if(moveVec != sf::Vector2f({0.f, 0.f}))
		{
			constexpr float tankMoveSpeed = 5.f;
			// there was movement
			// new center coordinates
			sf::Vector2f const TankPosNew = tankSprite.getPosition() + moveVec.normalized() * tankMoveSpeed;
			// we need top-left coordinates for the Rect
			sf::FloatRect newTankRect({TankPosNew.x - tankW / 2.f, TankPosNew.y - tankH / 2.f}, {tankW, tankH});

			bool collision = false;
			for(auto &wall : walls)
			{
				if(newTankRect.findIntersection(wall.getGlobalBounds()) != std::nullopt)
				{
					collision = true;
					break;
				}
			}

			if(!collision)
			{ // no collision, let them move there
				tankSprite.setPosition(TankPosNew);
			}
			else
			{	// player crashed against something, deal them damage
				hud.setHealth(hud.getHealth() - 1);
			}

			// face towards movement direction (0 deg = up, right hand side coordinate system)
			// atan2 preserves signs as it takes a 2D vector, which atan can't as it only takes the ratio.
			float const angRad = std::atan2(moveVec.x, -moveVec.y);
			tankSprite.setRotation(sf::radians(angRad));
		}

		float frameTimeSec = frameClock.restart().asSeconds();

		// Draw world
		window.setView(worldView);
		window.clear(sf::Color::White);
		for(auto &wall : walls)
			window.draw(wall);
		window.draw(tankSprite);

		// Draw hud
		hud.update(frameTimeSec);
		window.setView(hudView);
		window.draw(hud);

		window.display();
	}

	return 0;
}
