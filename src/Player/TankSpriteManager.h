#pragma once
#include "../ResourceManager.h"
#include "../SpriteSheet.h"
#include <memory>

class TankSpriteManager
{
  public:
	enum class TankState
	{
		HEALTHY,
		DAMAGED,
		DEAD
	};

	enum class TankPart
	{
		HULL,
		TURRET
	};

	TankSpriteManager(TankSpriteManager const &) = delete;
	TankSpriteManager &operator=(TankSpriteManager const &) = delete;

	static TankSpriteManager &inst()
	{
		static TankSpriteManager s_instance;
		return s_instance;
	}

	[[nodiscard]] sf::Sprite getSprite(TankPart part, TankState state, int direction) const
	{
		int index = getSpriteIndex(part, state, direction);
		return m_spriteSheet->getSprite(index);
	}

	[[nodiscard]] SpriteSheet const &getSpriteSheet() const
	{
		return *m_spriteSheet;
	}

  private:
	TankSpriteManager()
	{
		sf::Texture &texture = TextureManager::inst().load("tanks/tiger/hull_spritesheet.png");
		m_spriteSheet = std::make_unique<SpriteSheet>(texture, SPRITE_SIZE, SPRITE_SIZE, COLUMNS);
	}

	[[nodiscard]] int getSpriteIndex(TankPart part, TankState state, int direction) const
	{
		int row = 0;

		if(part == TankPart::HULL)
		{
			switch(state)
			{
			case TankState::HEALTHY:
				row = HEALTHY_HULL_ROW;
				break;
			case TankState::DAMAGED:
				row = DAMAGED_HULL_ROW;
				break;
			case TankState::DEAD:
				row = DEAD_HULL_ROW;
				break;
			}
		}
		else
		{
			row = TURRET_ROW;
		}

		return row * COLUMNS + direction;
	}

	std::unique_ptr<SpriteSheet> m_spriteSheet;

	static constexpr int COLUMNS = 8;
	static constexpr int SPRITE_SIZE = 150;
	static constexpr int HEALTHY_HULL_ROW = 0;
	static constexpr int DAMAGED_HULL_ROW = 1;
	static constexpr int DEAD_HULL_ROW = 2;
	static constexpr int TURRET_ROW = 3;
};
