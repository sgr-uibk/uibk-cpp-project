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

	float m_bobPhase;
	float m_bobSpeed;
	float m_bobHeight;
};
