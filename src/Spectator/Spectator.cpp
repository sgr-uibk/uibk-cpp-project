#include "Spectator.h"
#include <cmath>

Spectator::Spectator(sf::Vector2f initialPos)
    : m_pos(initialPos), m_moveSpeed(200.f)
{
    if (!m_texture.loadFromFile("../assets/spectator_icon.png"))
    {
        return;
    }

    m_sprite.emplace(m_texture);
    m_sprite->setOrigin({m_texture.getSize().x / 2.f, m_texture.getSize().y / 2.f});

    sf::Vector2u texSize = m_texture.getSize();
    m_baseScale = {75.f / texSize.x, 75.f / texSize.y};
    m_sprite->setScale(m_baseScale);
}

void Spectator::update(float dt, sf::Vector2f posDelta, sf::Vector2f mousePos)
{
    float length = std::sqrt(posDelta.x * posDelta.x + posDelta.y * posDelta.y);
    if (length > 1.f)
        posDelta /= length;

    m_pos += posDelta * m_moveSpeed * dt;

    if (!m_sprite.has_value())
        return;

    m_sprite->setPosition(m_pos);

    float dx = mousePos.x - m_pos.x;

    const float deadZone = 2.f;

    if (dx > deadZone)
    {
        m_sprite->setScale({-std::abs(m_baseScale.x), m_baseScale.y});
    }
    else if (dx < -deadZone)
    {
        m_sprite->setScale({std::abs(m_baseScale.x), m_baseScale.y});
    }
}


void Spectator::draw(sf::RenderWindow &window) const
{
    if (m_sprite.has_value())
        window.draw(*m_sprite);
}
