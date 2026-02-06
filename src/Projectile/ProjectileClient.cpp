#include "ProjectileClient.h"
#include "Utilities.h"

ProjectileClient::ProjectileClient(ProjectileState const &state) : m_state(state), m_shape(VISUAL_RADIUS)
{
	m_shape.setFillColor(FILL_COLOR);
	m_shape.setOrigin(sf::Vector2f(VISUAL_RADIUS, VISUAL_RADIUS));
	syncSpriteToState(state);
}

void ProjectileClient::syncSpriteToState(ProjectileState const &state)
{
	m_state = state;
	m_shape.setPosition(cartesianToIso(m_state.getPosition()));
}

void ProjectileClient::update([[maybe_unused]] float dt)
{
}
