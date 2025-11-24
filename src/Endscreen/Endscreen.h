#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include "Player/PlayerState.h"

class Endscreen
{
public:
    Endscreen(const sf::Vector2u &windowSize);

    void setup(const PlayerState &winner, const std::vector<PlayerState> &players);

    void draw(sf::RenderWindow &window) const;

    void update(float dt);

    bool wantsRestart() const { return m_restart; }
    bool wantsExit() const { return m_exit; }
    bool shouldExit() const { return m_exit; }

private:
    sf::Font m_font;
    std::unique_ptr<sf::Text> m_title;
    std::unique_ptr<sf::Text> m_placementText;
    std::unique_ptr<sf::Text> m_continueText;

    bool m_setupDone = false;

    bool m_restart = false;
    bool m_exit = false;

    float m_timer = 3.0f;
};
