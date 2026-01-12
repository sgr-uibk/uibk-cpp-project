#pragma once
#include "../Powerup.h"
#include "../ResourceManager.h"
#include "../SpriteSheet.h"
#include <memory>

class PowerupSpriteManager
{
  public:
	PowerupSpriteManager(PowerupSpriteManager const &) = delete;
	PowerupSpriteManager &operator=(PowerupSpriteManager const &) = delete;

	static PowerupSpriteManager &inst()
	{
		static PowerupSpriteManager s_instance;
		return s_instance;
	}

	[[nodiscard]] sf::Sprite getSpriteForType(PowerupType type, float scale = 0.5f) const
	{
		int index = getSpriteIndex(type);
		sf::Sprite sprite = m_spriteSheet->getSprite(index);

		sprite.setOrigin(sf::Vector2f(75.0f, 75.0f));
		sprite.setScale(sf::Vector2f(scale, scale));

		return sprite;
	}

	[[nodiscard]] SpriteSheet const &getSpriteSheet() const
	{
		return *m_spriteSheet;
	}

  private:
	PowerupSpriteManager()
	{
		sf::Texture &texture = TextureManager::inst().load("powerups/spawn_powerups.png");
		m_spriteSheet = std::make_unique<SpriteSheet>(texture, 150, 150, 5);
	}

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
			return 0;
		}
	}

	std::unique_ptr<SpriteSheet> m_spriteSheet;
};
