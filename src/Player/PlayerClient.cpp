#include "PlayerClient.h"
#include "Utilities.h"
#include "ResourceManager.h"
#include <algorithm>

static int angleToDirection(float degrees)
{
	sf::Angle angle = sf::degrees(degrees).wrapUnsigned();
	float normalized = angle.asDegrees();

	float adjusted = std::fmod(normalized + 22.5f, 360.f);
	int section = static_cast<int>(adjusted / 45.f);

	// Hardcoded sprite mapping for now
	int const spriteMap[8] = {3, 2, 1, 8, 7, 6, 5, 4};
	return spriteMap[section];
}

PlayerClient::PlayerClient(PlayerState &state, sf::Color const &color)
	: m_state(state), m_color(color),
	  m_hullSprite(TankSpriteManager::inst().getSprite(TankSpriteManager::TankPart::HULL, TankSpriteManager::TankState::HEALTHY, 0)),
	  m_turretSprite(TankSpriteManager::inst().getSprite(TankSpriteManager::TankPart::TURRET, TankSpriteManager::TankState::HEALTHY, 0)),
	  m_currentTankState(TankSpriteManager::TankState::HEALTHY),
	  m_font(FontManager::inst().load("Font/LiberationSans-Regular.ttf")), m_nameText(m_font, m_state.m_name, 14),
	  m_shootAnimTimer(0.f), m_lastShootCooldown(0.f)
{
	// Properly initialize sprites based on current state
	syncSpriteToState();

	m_nameText.setFillColor(sf::Color::White);
	m_nameText.setOutlineColor(sf::Color::Black);
	m_nameText.setOutlineThickness(1.f);
	updateNameText();
}

void PlayerClient::update(float const dt)
{
	float currentCooldown = m_state.m_shootCooldown.getRemaining();
	if(m_lastShootCooldown == 0.f && currentCooldown > 0.f)
	{
		m_shootAnimTimer = SHOOT_ANIM_DURATION;
	}
	m_lastShootCooldown = currentCooldown;
	m_shootAnimTimer = std::clamp(m_shootAnimTimer - dt, 0.f, SHOOT_ANIM_DURATION);

	// smoothing / interpolation can be inserted here
	syncSpriteToState();
	updateNameText();
}

void PlayerClient::applyServerState(PlayerState const &serverState)
{
	int prevHealth = m_state.m_health;
	m_state.assignSnappedState(serverState);
	if(m_onHealthChanged && prevHealth != m_state.m_health)
	{
		m_onHealthChanged(m_state.m_health, m_state.m_maxHealth);
	}
	syncSpriteToState();
}

void PlayerClient::applyLocalMove(MapState const &map, sf::Vector2f delta)
{
	m_state.moveOn(map, delta);
	syncSpriteToState();
}

void PlayerClient::setTurretRotation(sf::Angle angle)
{
	m_state.setCannonRotation(angle);
	syncSpriteToState();
}

void PlayerClient::registerHealthCallback(HealthCallback cb)
{
	m_onHealthChanged = std::move(cb);
}

sf::Vector2f lerp(sf::Vector2f const &a, sf::Vector2f const &b, float t)
{
	return a + t * (b - a);
}

sf::Angle lerp(sf::Angle const &a, sf::Angle const &b, float t)
{
	auto const delta = (b - a).wrapSigned();
	return a + t * delta;
}

void PlayerClient::interp(InterpPlayerState const &s0, InterpPlayerState const &s1, float const alpha)
{
	this->m_state.m_iState.m_pos = lerp(s0.m_pos, s1.m_pos, alpha);
	this->m_state.m_iState.m_rot = lerp(s0.m_rot, s1.m_rot, alpha);
	m_state.m_iState.m_cannonRot = lerp(s0.m_cannonRot, s1.m_cannonRot, alpha);
}

void PlayerClient::overwriteInterpState(InterpPlayerState authState)
{
	m_state.m_iState.overwriteBy(authState);
}

void PlayerClient::updateSprite()
{
	syncSpriteToState();
}

void PlayerClient::syncSpriteToState()
{
	// Determine tank health state (50% threshold)
	TankSpriteManager::TankState newState = (m_state.m_health > m_state.m_maxHealth / 2)
	                                            ? TankSpriteManager::TankState::HEALTHY
	                                            : TankSpriteManager::TankState::DAMAGED;

	// Calculate direction indices (existing logic)
	int hullDir = angleToDirection(m_state.getRotation().asDegrees()) - 1;
	int turretDir = angleToDirection(m_state.getCannonRotation().asDegrees() - 135.f) - 1;
	hullDir = std::clamp(hullDir, 0, 7);
	turretDir = std::clamp(turretDir, 0, 7);

	// Update state tracking
	m_currentTankState = newState;

	// Get sprites from manager
	m_hullSprite = TankSpriteManager::inst().getSprite(TankSpriteManager::TankPart::HULL, m_currentTankState, hullDir);
	m_turretSprite = TankSpriteManager::inst().getSprite(
	    TankSpriteManager::TankPart::TURRET,
	    TankSpriteManager::TankState::HEALTHY, // Turret always healthy (row 2)
	    turretDir);

	// Set sprite origins (150x150 sprites, origin at center)
	m_hullSprite.setOrigin(sf::Vector2f(75.f, 75.f));
	m_turretSprite.setOrigin(sf::Vector2f(75.f, 75.f));

	// Position and color (existing logic)
	sf::Vector2f cartCenter = m_state.getPosition() + sf::Vector2f(PlayerState::logicalDimensions / 2.f);
	sf::Vector2f isoCenter = cartesianToIso(cartCenter);
	m_hullSprite.setPosition(isoCenter);
	m_turretSprite.setPosition(isoCenter);

	m_hullSprite.setColor(m_color);
	m_turretSprite.setColor(m_color);

	// Dead tank rendering (unchanged)
	if(m_state.m_health <= 0)
	{
		sf::Color deadColor = m_color;
		deadColor.a = 128;
		m_hullSprite.setColor(deadColor);
		m_turretSprite.setColor(deadColor);
	}
}

void PlayerClient::updateNameText()
{
	sf::FloatRect textBounds = m_nameText.getLocalBounds();
	sf::Vector2f cartTankCenter = m_state.m_iState.m_pos + sf::Vector2f(PlayerState::logicalDimensions / 2.f);

	sf::Vector2f isoTankCenter = cartesianToIso(cartTankCenter);

	sf::Vector2f textPos(isoTankCenter.x - textBounds.size.x / 2.f - textBounds.position.x,
	                     isoTankCenter.y - tankDimensions.y / 2.f - 18.f // 18 pixels above tank
	);
	m_nameText.setPosition(textPos);
}

void PlayerClient::collectRenderObjects(std::vector<RenderObject> &queue) const
{
	sf::Vector2f isoPos = m_hullSprite.getPosition();
	float depthY = isoPos.y;

	RenderObject hullObj;
	hullObj.sortY = depthY;
	hullObj.drawable = &m_hullSprite;
	queue.push_back(hullObj);

	RenderObject turretObj;
	turretObj.sortY = depthY + 0.1f;
	turretObj.drawable = &m_turretSprite;
	queue.push_back(turretObj);

	RenderObject textObj;
	textObj.sortY = depthY + 1.0f;
	textObj.drawable = &m_nameText;
	queue.push_back(textObj);
}
