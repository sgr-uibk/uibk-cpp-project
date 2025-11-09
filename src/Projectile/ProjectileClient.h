#pragma once
#include <SFML/Graphics.hpp>
#include "ProjectileState.h"

class ProjectileClient
{
  public:
	ProjectileClient(const ProjectileState &state);

	void syncSpriteToState(const ProjectileState &state);

	void draw(sf::RenderWindow &window) const;

	void update(float dt);

	uint32_t getId() const
	{
		return m_state.getId();
	}

  private:
	ProjectileState m_state;
	sf::CircleShape m_shape;
};
