#pragma once
#include <SFML/Graphics.hpp>
#include "ProjectileState.h"

class ProjectileClient
{
  public:
	ProjectileClient(const ProjectileState &state);

	void syncSpriteToState(const ProjectileState &state);

	void update(float dt);

	uint32_t getId() const
	{
		return m_state.getId();
	}

	const sf::CircleShape &getShape() const
	{
		return m_shape;
	}

	const ProjectileState &getState() const
	{
		return m_state;
	}

  private:
	ProjectileState m_state;
	sf::CircleShape m_shape;
};
