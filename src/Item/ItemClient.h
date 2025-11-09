#pragma once
#include <SFML/Graphics.hpp>
#include "ItemState.h"

class ItemClient
{
  public:
	ItemClient(const ItemState &state);

	void syncSpriteToState(const ItemState &state);

	void draw(sf::RenderWindow &window) const;

	void update(float dt);

	uint32_t getId() const
	{
		return m_state.getId();
	}

  private:
	ItemState m_state;
	sf::RectangleShape m_shape;

	float m_bobPhase;
	float m_bobSpeed;
	float m_bobHeight;

	static sf::Color getColorForType(PowerupType type);
};
