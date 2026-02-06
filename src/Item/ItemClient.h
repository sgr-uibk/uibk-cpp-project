#pragma once
#include <SFML/Graphics.hpp>
#include "ItemState.h"

class ItemClient
{
  public:
	ItemClient(ItemState const &state);

	void syncSpriteToState(ItemState const &state);

	void update(float dt);

	uint32_t getId() const
	{
		return m_state.getId();
	}

	sf::Sprite const &getSprite() const
	{
		return m_sprite;
	}

	ItemState const &getState() const
	{
		return m_state;
	}

  private:
	ItemState m_state;
	sf::Sprite m_sprite;

	float m_bobPhase = 0.f;

	static constexpr float BOB_SPEED = 0.05f; // oscillation frequency in Hz
	static constexpr float BOB_HEIGHT = 5.f;  // vertical displacement in pixels
};
