#include <SFML/Graphics.hpp>
#include <cmath>
#include <optional>

#include "HealthBar.h"
#include "GameWorld.h"

void updateView(sf::RenderWindow& window, sf::View& view, sf::Vector2u baseSize)
{
    float windowWidth  = static_cast<float>(window.getSize().x);
    float windowHeight = static_cast<float>(window.getSize().y);
    float windowRatio  = windowWidth / windowHeight;
    float baseRatio    = static_cast<float>(baseSize.x) / static_cast<float>(baseSize.y);

    float viewportX = 0.f;
    float viewportY = 0.f;
    float viewportWidth = 1.f;
    float viewportHeight = 1.f;

    if (windowRatio > baseRatio)
    {
        viewportWidth = baseRatio / windowRatio;
        viewportX = (1.f - viewportWidth) / 2.f;
    }
    else
    {
        viewportHeight = windowRatio / baseRatio;
        viewportY = (1.f - viewportHeight) / 2.f;
    }

    view.setViewport(sf::FloatRect({viewportX, viewportY}, {viewportWidth, viewportHeight}));
    view.setSize({static_cast<float>(baseSize.x), static_cast<float>(baseSize.y)});
    view.setCenter({static_cast<float>(baseSize.x) / 2.f, static_cast<float>(baseSize.y) / 2.f});
}


int main()
{
    sf::Vector2u const baseResolution = {800, 600};

    sf::RenderWindow window(sf::VideoMode(baseResolution), "Tank Game", sf::Style::Resize);
    window.setFramerateLimit(60);
    window.setMinimumSize(baseResolution);
    window.setMaximumSize(std::optional(sf::Vector2u{3840, 2160}));

    GameWorld gameWorld{baseResolution};
    sf::Clock frameClock;

    updateView(window, gameWorld.getWorldView(), baseResolution);

    sf::Vector2u lastSize = window.getSize();

    while (window.isOpen())
    {
        while (const std::optional event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
                window.close();

            else if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>())
            {
                if (keyPressed->scancode == sf::Keyboard::Scancode::Escape)
                    window.close();
            }
        }

        sf::Vector2u currentSize = window.getSize();
        if (currentSize != lastSize)
        {
            lastSize = currentSize;
            updateView(window, const_cast<sf::View&>(gameWorld.getWorldView()), baseResolution);
        }

        float const dt = frameClock.restart().asSeconds();
        gameWorld.update(dt);

        window.clear(sf::Color::Black);
        window.setView(gameWorld.getWorldView());

        gameWorld.draw(window);

        window.display();
    }

    return 0;
}
