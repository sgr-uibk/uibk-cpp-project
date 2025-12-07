#pragma once
#include <SFML/Graphics.hpp>
#include "ItemState.h"

class ItemClient
{
  public:
	ItemClient(const ItemState &state);

	void syncSpriteToState(const ItemState &state);

	void update(float dt);

	uint32_t getId() const
	{
		return m_state.getId();
	}

	const sf::RectangleShape &getShape() const
	{
		return m_shape;
	}

	const ItemState &getState() const
	{
		return m_state;
	}

  private:
	ItemState m_state;
	sf::RectangleShape m_shape;

	float m_bobPhase;
	float m_bobSpeed;
	float m_bobHeight;

	static sf::Color getColorForType(PowerupType type);
};
