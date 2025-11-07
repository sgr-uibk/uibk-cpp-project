#include "Tile.h"

Tile::Tile(TileType type, sf::Vector2f pos, float size) 
    : m_type(type), m_rect({size, size})
{
    m_rect.setPosition(pos);
}

void Tile::setTexture(const sf::Texture &texture)
{
    m_sprite.emplace(texture);
    m_sprite->setPosition(m_rect.getPosition());
    m_sprite->setScale({
        m_rect.getSize().x / texture.getSize().x,
        m_rect.getSize().y / texture.getSize().y}
    );
    m_hasTexture = true;
}

void Tile::draw(sf::RenderWindow &window) const
{
    if(m_hasTexture && m_sprite)
        window.draw(*m_sprite);
    else
        window.draw(m_rect);
}
