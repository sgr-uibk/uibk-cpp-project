#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <optional>

enum class TileType { Wall, Grass, Water, Stone };

class Tile
{
public:
    Tile(TileType type, sf::Vector2f pos, float size);

    void setTexture(const sf::Texture &texture);
    void draw(sf::RenderWindow &window) const;

    TileType getType() const { return m_type; }
    sf::Vector2f getPosition() const { return m_rect.getPosition(); }

private:
    TileType m_type;
    sf::RectangleShape m_rect;
    std::optional<sf::Sprite> m_sprite;
    bool m_hasTexture = false;
};
