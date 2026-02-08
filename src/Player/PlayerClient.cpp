#include "PlayerClient.h"
#include "Utilities.h"
#include "ResourceManager.h"
#include <algorithm>

static int angleToSpriteColumn(float degrees)
{
	sf::Angle angle = sf::degrees(degrees).wrapUnsigned();
	float normalized = angle.asDegrees();

	float adjusted = std::fmod(normalized + GameConfig::TankSprite::HALF_SECTOR, 360.f);
	int sector = static_cast<int>(adjusted / GameConfig::TankSprite::DEGREES_PER_SECTOR);

	return GameConfig::TankSprite::DIRECTION_TO_SPRITE[sector];
}

PlayerClient::PlayerClient(PlayerState &state, sf::Color const &color)
	: m_state(state), m_color(color), m_hullSprite(TankSpriteManager::inst().getSprite(
										  TankSpriteManager::TankPart::HULL, TankSpriteManager::TankState::HEALTHY, 0)),
	  m_turretSprite(TankSpriteManager::inst().getSprite(TankSpriteManager::TankPart::TURRET,
                                                         TankSpriteManager::TankState::HEALTHY, 0)),
	  m_currentTankState(TankSpriteManager::TankState::HEALTHY),
	  m_font(FontManager::inst().load("Font/LiberationSans-Regular.ttf")), m_nameText(m_font, m_state.m_name, 14),
	  m_shootSoundBuf(SoundBufferManager::inst().load("audio/tank_firing.ogg")), m_shootSound(m_shootSoundBuf),
	  m_healthBar({0, 0}, {40.f, 5.f}, m_state.m_maxHealth)
{
	syncSpriteToState();

	m_nameText.setFillColor(sf::Color::White);
	m_nameText.setOutlineColor(sf::Color::Black);
	m_nameText.setOutlineThickness(1.f);
	updateNameText();

	m_healthBar.setTextVisible(false);
}

void PlayerClient::update(float const dt)
{
	float currentCooldown = m_state.m_shootCooldown.getRemaining();
	if(m_lastShootCooldown == 0.f && currentCooldown > 0.f)
	{
		m_shootAnimTimer = SHOOT_ANIM_DURATION;
		playShotSound();
	}
	m_lastShootCooldown = currentCooldown;
	m_shootAnimTimer = std::clamp(m_shootAnimTimer - dt, 0.f, SHOOT_ANIM_DURATION);

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

void PlayerClient::setCannonRotation(sf::Angle angle)
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
void PlayerClient::playShotSound()
{
	m_shootSound.play();
}

void PlayerClient::syncSpriteToState()
{
	if(m_state.m_health <= 0)
		m_currentTankState = TankSpriteManager::TankState::DEAD;
	else if(m_state.m_health > m_state.m_maxHealth / 2)
		m_currentTankState = TankSpriteManager::TankState::HEALTHY;
	else
		m_currentTankState = TankSpriteManager::TankState::DAMAGED;

	int hullDir = std::clamp(angleToSpriteColumn(m_state.getRotation().asDegrees()) - 1, 0, 7);
	float turretDegrees = m_state.getCannonRotation().asDegrees() + GameConfig::TankSprite::TURRET_ANGLE_OFFSET;
	int turretDir = std::clamp(angleToSpriteColumn(turretDegrees) - 1, 0, 7);

	m_hullSprite = TankSpriteManager::inst().getSprite(TankSpriteManager::TankPart::HULL, m_currentTankState, hullDir);
	m_turretSprite = TankSpriteManager::inst().getSprite(TankSpriteManager::TankPart::TURRET,
	                                                     TankSpriteManager::TankState::HEALTHY, turretDir);

	constexpr float origin = GameConfig::Player::TANK_SPRITE_ORIGIN;
	m_hullSprite.setOrigin({origin, origin});
	m_turretSprite.setOrigin({origin, origin});

	sf::Vector2f cartCenter = m_state.getPosition() + sf::Vector2f(PlayerState::logicalDimensions / 2.f);
	sf::Vector2f isoCenter = cartesianToIso(cartCenter);
	m_hullSprite.setPosition(isoCenter);
	m_turretSprite.setPosition(isoCenter);

	m_hullSprite.setColor(m_color);
	m_turretSprite.setColor(m_color);

	m_healthBar.setMaxHealth(m_state.m_maxHealth);
	m_healthBar.setHealth(m_state.m_health);
	sf::Vector2f barPos(isoCenter.x - 20.f, isoCenter.y - tankDimensions.y / 2.f - 6.f);
	m_healthBar.setPositionScreen(barPos);
}

void PlayerClient::updateNameText()
{
	if(m_nameText.getString() != m_state.m_name)
		m_nameText.setString(m_state.m_name);

	sf::FloatRect textBounds = m_nameText.getLocalBounds();
	sf::Vector2f cartTankCenter = m_state.m_iState.m_pos + sf::Vector2f(PlayerState::logicalDimensions / 2.f);

	sf::Vector2f isoTankCenter = cartesianToIso(cartTankCenter);

	sf::Vector2f textPos(isoTankCenter.x - textBounds.size.x / 2.f - textBounds.position.x,
	                     isoTankCenter.y - tankDimensions.y / 2.f - 26.f);
	m_nameText.setPosition(textPos);
}

void PlayerClient::collectRenderObjects(std::vector<RenderObject> &queue, EntityId ownPlayerId) const
{
	sf::Vector2f cartFrontCorner = m_state.getPosition() + PlayerState::logicalDimensions;
	float depthY = cartesianToIso(cartFrontCorner).y;

	RenderObject hullObj;
	hullObj.sortY = depthY;
	hullObj.drawable = &m_hullSprite;
	queue.push_back(hullObj);

	if(m_currentTankState != TankSpriteManager::TankState::DEAD)
	{
		RenderObject turretObj;
		turretObj.sortY = depthY + 0.1f;
		turretObj.drawable = &m_turretSprite;
		queue.push_back(turretObj);
	}

	RenderObject textObj;
	textObj.sortY = depthY + 1.0f;
	textObj.drawable = &m_nameText;
	queue.push_back(textObj);

	if(m_state.m_id != ownPlayerId)
	{
		RenderObject healthBarObj;
		healthBarObj.sortY = depthY + 1.1f;
		healthBarObj.drawable = &m_healthBar;
		queue.push_back(healthBarObj);
	}
}

void PlayerClient::drawSilhouette(sf::RenderWindow &window, std::uint8_t alpha) const
{
	sf::Sprite hullSilhouette = m_hullSprite;
	sf::Sprite turretSilhouette = m_turretSprite;

	sf::Color silhouetteColor = m_color;
	silhouetteColor.a = alpha;

	hullSilhouette.setColor(silhouetteColor);
	turretSilhouette.setColor(silhouetteColor);

	window.draw(hullSilhouette);
	window.draw(turretSilhouette);
}
