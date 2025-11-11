#pragma once

#include <SFML/Graphics.hpp>
#include <cmath>
#include <optional>

class Spectator
{
public:
    Spectator(sf::Vector2f initialPos);

    void update(float dt, sf::Vector2f posDelta, sf::Vector2f mousePos);
    void draw(sf::RenderWindow &window) const;
    void setPosition(sf::Vector2f pos) { m_pos = pos; }

    [[nodiscard]] sf::Vector2f getPosition() const
    {
        return m_pos;
    }

private:
    sf::Vector2f m_pos;
    sf::Angle m_rot;
    float m_moveSpeed = 400.f;
    sf::Texture m_texture;
    std::optional<sf::Sprite> m_sprite;
    sf::Vector2f m_baseScale;
};