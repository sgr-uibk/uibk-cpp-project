#include "ProjectileClient.h"
#include <cmath>

ProjectileClient::ProjectileClient(const ProjectileState &state) : m_state(state), m_shape(4.f)
{
	m_shape.setFillColor(sf::Color::Black);
	m_shape.setOrigin(sf::Vector2f(4.f, 4.f));
	syncSpriteToState(state);
}

void ProjectileClient::syncSpriteToState(const ProjectileState &state)
{
	m_state = state;
	m_shape.setPosition(m_state.getPosition());
}

void ProjectileClient::update([[maybe_unused]] float dt)
{
	// visual updates could be implemented in the future
}

void ProjectileClient::draw(sf::RenderWindow &window) const
{
	if(m_state.isActive())
	{
		window.draw(m_shape);
	}
}
