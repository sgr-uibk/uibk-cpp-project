#include "AmmunitionDisplayClient.h"
#include <algorithm>
#include <cmath>

AmmunitionDisplay::AmmunitionDisplay(PlayerState const &playerState, sf::RenderWindow const &window)
	: m_playerState(playerState), m_window(window)
{
	for(auto &bullet : m_bullets)
	{
		bullet.state = BulletSlot::State::FILLED;
		bullet.animProgress = 0.f;
	}
}

void AmmunitionDisplay::update(float dt)
{
	float currentCooldown = m_playerState.m_shootCooldown.getRemaining();
	if(m_prevCooldownRemaining == 0.f && currentCooldown > 0.f)
	{
		onShoot();
	}
	m_prevCooldownRemaining = currentCooldown;

	m_flashTimer = std::max(0.f, m_flashTimer - dt);

	for(auto &bullet : m_bullets)
	{
		if(bullet.state == BulletSlot::State::DEPLETING)
		{
			bullet.animProgress += dt / DEPLETING_ANIM_DURATION;
			if(bullet.animProgress >= 1.f)
			{
				bullet.state = BulletSlot::State::EMPTY;
				bullet.animProgress = 0.f;
			}
		}
	}

	if(m_currentReloadBullet == -1)
	{
		for(size_t i = 0; i < m_bullets.size(); ++i)
		{
			if(m_bullets[i].state == BulletSlot::State::EMPTY)
			{
				m_currentReloadBullet = static_cast<int>(i);
				break;
			}
		}
	}

	if(m_currentReloadBullet != -1)
	{
		auto &bullet = m_bullets[m_currentReloadBullet];

		if(bullet.state == BulletSlot::State::FILLED)
		{
			m_currentReloadBullet = -1;
			m_reloadProgress = 0.f;
		}
		else if(bullet.state == BulletSlot::State::EMPTY || bullet.state == BulletSlot::State::REFILLING)
		{
			float reloadDuration = m_playerState.getShootCooldown() * 4.0f;
			m_reloadProgress += dt / reloadDuration;

			bullet.state = BulletSlot::State::REFILLING;
			bullet.animProgress = m_reloadProgress;

			if(m_reloadProgress >= 1.f)
			{
				bullet.state = BulletSlot::State::FILLED;
				bullet.animProgress = 0.f;
				m_reloadProgress = 0.f;
				m_currentReloadBullet = -1;
			}
		}
	}
	else
	{
		m_reloadProgress = 0.f;
	}
}

void AmmunitionDisplay::onShoot()
{
	for(int i = BULLET_COUNT - 1; i >= 0; --i)
	{
		if(m_bullets[i].state == BulletSlot::State::FILLED)
		{
			m_bullets[i].state = BulletSlot::State::DEPLETING;
			m_bullets[i].animProgress = 0.f;

			if(m_currentReloadBullet != -1 && m_currentReloadBullet != i)
			{
				m_bullets[m_currentReloadBullet].state = BulletSlot::State::EMPTY;
				m_bullets[m_currentReloadBullet].animProgress = 0.f;
			}

			m_currentReloadBullet = i;
			break;
		}
	}
}

void AmmunitionDisplay::draw(sf::RenderWindow &window) const
{
	sf::Vector2f panelPos = calculatePanelPosition();
	sf::Vector2f panelSize = calculatePanelSize();

	drawMetalPlateBackground(window, panelPos, panelSize);

	float bulletX = panelPos.x + PANEL_PADDING;
	float bulletY = panelPos.y + PANEL_PADDING;

	for(size_t i = 0; i < m_bullets.size(); ++i)
	{
		drawBullet(window, i, {bulletX, bulletY});
		bulletX += BULLET_WIDTH + BULLET_SPACING;
	}
}

void AmmunitionDisplay::drawMetalPlateBackground(sf::RenderWindow &window, sf::Vector2f const &pos,
                                                 sf::Vector2f const &size) const
{
	using namespace GameConfig::UI;

	sf::RectangleShape plate(size);
	plate.setPosition(pos);
	plate.setFillColor(METAL_PLATE_COLOR);
	plate.setOutlineColor(METAL_PLATE_BORDER);
	plate.setOutlineThickness(METAL_PLATE_OUTLINE_THICKNESS);
	window.draw(plate);

	float const inset = METAL_HIGHLIGHT_INSET;
	sf::RectangleShape highlight({size.x - inset * 2.f, METAL_HIGHLIGHT_HEIGHT});
	highlight.setPosition({pos.x + inset, pos.y + inset});
	highlight.setFillColor(METAL_PLATE_HIGHLIGHT);
	window.draw(highlight);

	drawRivet(window, {pos.x + RIVET_INSET, pos.y + RIVET_INSET});
	drawRivet(window, {pos.x + size.x - RIVET_INSET, pos.y + RIVET_INSET});
	drawRivet(window, {pos.x + RIVET_INSET, pos.y + size.y - RIVET_INSET});
	drawRivet(window, {pos.x + size.x - RIVET_INSET, pos.y + size.y - RIVET_INSET});
}

void AmmunitionDisplay::drawBullet(sf::RenderWindow &window, size_t index, sf::Vector2f const &pos) const
{
	auto const &bullet = m_bullets[index];

	sf::RectangleShape casingBg({BULLET_WIDTH, BULLET_HEIGHT});
	casingBg.setPosition(pos);
	casingBg.setFillColor(sf::Color(25, 25, 25, 200));
	window.draw(casingBg);

	sf::Color brassColor(200, 160, 80);
	sf::Color tipColor(180, 140, 60);

	auto drawFullBullet = [&](sf::Color casingColor, sf::Color bulletTipColor) {
		sf::RectangleShape casing({BULLET_WIDTH, BULLET_HEIGHT - 6.f});
		casing.setPosition({pos.x, pos.y + 6.f});
		casing.setFillColor(casingColor);
		window.draw(casing);

		sf::ConvexShape tip(3);
		tip.setPoint(0, {pos.x, pos.y + 6.f});
		tip.setPoint(1, {pos.x + BULLET_WIDTH, pos.y + 6.f});
		tip.setPoint(2, {pos.x + BULLET_WIDTH / 2.f, pos.y});
		tip.setFillColor(bulletTipColor);
		window.draw(tip);
	};

	switch(bullet.state)
	{
	case BulletSlot::State::FILLED:
		drawFullBullet(brassColor, tipColor);
		break;

	case BulletSlot::State::EMPTY:
		break;

	case BulletSlot::State::DEPLETING: {
		uint8_t alpha = static_cast<uint8_t>(255 * (1.f - bullet.animProgress));
		sf::Color fadingBrass = brassColor;
		fadingBrass.a = alpha;
		sf::Color fadingTip = tipColor;
		fadingTip.a = alpha;
		drawFullBullet(fadingBrass, fadingTip);
		break;
	}

	case BulletSlot::State::REFILLING: {
		float fillHeight = (BULLET_HEIGHT - 6.f) * bullet.animProgress;

		bool isFlashing = m_flashTimer > 0.f && static_cast<int>(index) == m_currentReloadBullet;
		float flashIntensity = 0.f;
		if(isFlashing)
		{
			float pulsePhase = m_flashTimer * 8.f;
			flashIntensity = std::abs(std::sin(pulsePhase * 3.14159f)) * 0.8f;
		}

		sf::Color refillingColor = brassColor;
		if(flashIntensity > 0.f)
		{
			refillingColor.r = static_cast<uint8_t>(std::min(255.f, brassColor.r + 55.f * flashIntensity));
			refillingColor.g = static_cast<uint8_t>(brassColor.g * (1.f - flashIntensity * 0.6f));
			refillingColor.b = static_cast<uint8_t>(brassColor.b * (1.f - flashIntensity * 0.6f));
		}

		if(fillHeight > 0.f)
		{
			sf::RectangleShape casing({BULLET_WIDTH, fillHeight});
			casing.setPosition({pos.x, pos.y + BULLET_HEIGHT - fillHeight});
			casing.setFillColor(refillingColor);
			window.draw(casing);
		}

		if(bullet.animProgress > 0.85f)
		{
			float tipProgress = (bullet.animProgress - 0.85f) / 0.15f;
			sf::Color partialTip = tipColor;
			partialTip.a = static_cast<uint8_t>(255 * tipProgress);
			if(flashIntensity > 0.f)
			{
				partialTip.r = static_cast<uint8_t>(std::min(255.f, tipColor.r + 75.f * flashIntensity));
				partialTip.g = static_cast<uint8_t>(tipColor.g * (1.f - flashIntensity * 0.6f));
				partialTip.b = static_cast<uint8_t>(tipColor.b * (1.f - flashIntensity * 0.6f));
			}

			sf::ConvexShape tip(3);
			tip.setPoint(0, {pos.x, pos.y + 6.f});
			tip.setPoint(1, {pos.x + BULLET_WIDTH, pos.y + 6.f});
			tip.setPoint(2, {pos.x + BULLET_WIDTH / 2.f, pos.y});
			tip.setFillColor(partialTip);
			window.draw(tip);
		}
		break;
	}
	}
}

void AmmunitionDisplay::drawRivet(sf::RenderWindow &window, sf::Vector2f const &pos) const
{
	using namespace GameConfig::UI;

	sf::CircleShape rivet(RIVET_RADIUS);
	rivet.setOrigin({RIVET_RADIUS, RIVET_RADIUS});
	rivet.setPosition(pos);
	rivet.setFillColor(METAL_RIVET_COLOR);
	rivet.setOutlineColor(RIVET_OUTLINE_COLOR);
	rivet.setOutlineThickness(RIVET_OUTLINE_THICKNESS);
	window.draw(rivet);
}

sf::Vector2f AmmunitionDisplay::calculatePanelPosition() const
{
	sf::Vector2u windowSize = m_window.getSize();
	sf::Vector2f panelSize = calculatePanelSize();

	float x = static_cast<float>(windowSize.x) - panelSize.x - MARGIN;
	float y = static_cast<float>(windowSize.y) - panelSize.y - MARGIN;

	return {x, y};
}

sf::Vector2f AmmunitionDisplay::calculatePanelSize() const
{
	float width = PANEL_PADDING * 2.f + BULLET_COUNT * BULLET_WIDTH + (BULLET_COUNT - 1) * BULLET_SPACING;
	float height = PANEL_PADDING * 2.f + BULLET_HEIGHT;

	return {width, height};
}

bool AmmunitionDisplay::hasAmmo() const
{
	return std::any_of(m_bullets.begin(), m_bullets.end(),
	                   [](auto const &b) { return b.state == BulletSlot::State::FILLED; });
}

void AmmunitionDisplay::triggerEmptyFlash()
{
	m_flashTimer = FLASH_DURATION;
}
