#pragma once
#include "../Powerup.h"
#include "../ResourceManager.h"
#include "../SpriteSheet.h"
#include <memory>

/// @brief Singleton manager for powerup sprite sheet rendering
/// @details Loads the powerup sprite sheet once and provides sprites for each powerup type
class PowerupSpriteManager
{
  public:
	PowerupSpriteManager(PowerupSpriteManager const &) = delete;
	PowerupSpriteManager &operator=(PowerupSpriteManager const &) = delete;

	/// @brief Get the singleton instance
	static PowerupSpriteManager &inst()
	{
		static PowerupSpriteManager s_instance;
		return s_instance;
	}

	/// @brief Get a sprite for the specified powerup type
	/// @param type The powerup type
	/// @param scale The scale factor to apply to the sprite (default: 0.5 = half size ~35-40px)
	/// @return Sprite configured with correct texture rect, origin, and scale
	[[nodiscard]] sf::Sprite getSpriteForType(PowerupType type, float scale = 0.5f) const
	{
		int index = getSpriteIndex(type);
		sf::Sprite sprite = m_spriteSheet->getSprite(index);

		// Set origin to center of sprite (before scaling) - SFML 3.0 uses Vector2f
		sprite.setOrigin(sf::Vector2f(75.0f, 75.0f));

		// Scale to desired size - SFML 3.0 uses Vector2f
		sprite.setScale(sf::Vector2f(scale, scale));

		return sprite;
	}

	/// @brief Get the underlying sprite sheet
	[[nodiscard]] SpriteSheet const &getSpriteSheet() const
	{
		return *m_spriteSheet;
	}

  private:
	PowerupSpriteManager()
	{
		// Load powerup sprite sheet texture (1 row × 5 columns, 750x150 pixels)
		sf::Texture &texture = TextureManager::inst().load("powerups/spawn_powerups.png");

		// Create sprite sheet parser: 150x150 sprites, 5 columns
		m_spriteSheet = std::make_unique<SpriteSheet>(texture, 150, 150, 5);
	}

	/// @brief Map PowerupType to sprite sheet index
	/// @details Order: HEALTH_PACK(0), SPEED_BOOST(1), DAMAGE_BOOST(2), RAPID_FIRE(3), SHIELD(4)
	[[nodiscard]] static int getSpriteIndex(PowerupType type)
	{
		switch(type)
		{
		case PowerupType::HEALTH_PACK:
			return 0;
		case PowerupType::SPEED_BOOST:
			return 1;
		case PowerupType::DAMAGE_BOOST:
			return 2;
		case PowerupType::RAPID_FIRE:
			return 3;
		case PowerupType::SHIELD:
			return 4;
		default:
			return 0; // Default to HEALTH_PACK sprite
		}
	}

	std::unique_ptr<SpriteSheet> m_spriteSheet;
};
