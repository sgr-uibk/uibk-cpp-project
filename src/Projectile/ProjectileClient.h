#pragma once
#include <SFML/Graphics.hpp>
#include "ProjectileState.h"

class ProjectileClient
{
  public:
	ProjectileClient(ProjectileState const &state);

	void syncSpriteToState(ProjectileState const &state);

	void update(float dt);

	uint32_t getId() const
	{
		return m_state.getId();
	}

	sf::CircleShape const &getShape() const
	{
		return m_shape;
	}

	ProjectileState const &getState() const
	{
		return m_state;
	}

  private:
	ProjectileState m_state;
	sf::CircleShape m_shape;
};
