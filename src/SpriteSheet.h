#pragma once
#include <SFML/Graphics.hpp>

/// @brief Minimal sprite sheet parser for extracting sprites from a grid-based texture atlas
/// @details Single-header, thread-safe, immutable design for parsing uniform grid sprite sheets
class SpriteSheet
{
  public:
	/// @brief Construct a sprite sheet from a texture with uniform grid layout
	/// @param texture Reference to the loaded texture (must outlive SpriteSheet instance)
	/// @param spriteWidth Width of each sprite in pixels
	/// @param spriteHeight Height of each sprite in pixels
	/// @param columns Number of columns in the grid
	SpriteSheet(sf::Texture const &texture, int spriteWidth, int spriteHeight, int columns)
		: m_texture(texture), m_spriteWidth(spriteWidth), m_spriteHeight(spriteHeight), m_columns(columns)
	{
	}

	/// @brief Get a sprite at the specified grid index
	/// @param index Linear index (0-based, row-major order: left-to-right, top-to-bottom)
	/// @return Sprite configured with the correct texture rectangle
	[[nodiscard]] sf::Sprite getSprite(int index) const
	{
		int col = index % m_columns;
		int row = index / m_columns;

		// SFML 3.0 uses position and size vectors for IntRect
		sf::IntRect rect(sf::Vector2i(col * m_spriteWidth, row * m_spriteHeight),
		                 sf::Vector2i(m_spriteWidth, m_spriteHeight));

		sf::Sprite sprite(m_texture, rect);
		return sprite;
	}

	/// @brief Get sprite dimensions
	[[nodiscard]] sf::Vector2i getSpriteSize() const
	{
		return {m_spriteWidth, m_spriteHeight};
	}

  private:
	sf::Texture const &m_texture;
	int m_spriteWidth;
	int m_spriteHeight;
	int m_columns;
};
