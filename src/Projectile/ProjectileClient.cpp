#include "ProjectileClient.h"
#include "Utilities.h"
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
	sf::Vector2f isoPos = cartesianToIso(m_state.getPosition());
	m_shape.setPosition(isoPos);
}

void ProjectileClient::update(float dt)
{
	// visual updates could be implemented in the future
}