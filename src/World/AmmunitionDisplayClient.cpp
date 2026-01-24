#include "AmmunitionDisplayClient.h"
#include <algorithm>

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

	int bulletToReload = -1;
	for(size_t i = 0; i < m_bullets.size(); ++i)
	{
		if(m_bullets[i].state == BulletSlot::State::EMPTY || m_bullets[i].state == BulletSlot::State::REFILLING)
		{
			bulletToReload = static_cast<int>(i);
			break;
		}
	}

	if(bulletToReload != -1)
	{
		float reloadDuration = m_playerState.getShootCooldown() * 4.0f;
		m_reloadProgress += dt / reloadDuration;

		m_bullets[bulletToReload].state = BulletSlot::State::REFILLING;
		m_bullets[bulletToReload].animProgress = m_reloadProgress;

		if(m_reloadProgress >= 1.f)
		{
			m_bullets[bulletToReload].state = BulletSlot::State::FILLED;
			m_bullets[bulletToReload].animProgress = 0.f;
			m_reloadProgress = 0.f;
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
	sf::RectangleShape plate(size);
	plate.setPosition(pos);
	plate.setFillColor(GameConfig::UI::METAL_PLATE_COLOR);
	plate.setOutlineColor(sf::Color(40, 42, 45));
	plate.setOutlineThickness(2.f);
	window.draw(plate);

	sf::RectangleShape highlight({size.x - 4.f, 2.f});
	highlight.setPosition({pos.x + 2.f, pos.y + 2.f});
	highlight.setFillColor(GameConfig::UI::METAL_PLATE_HIGHLIGHT);
	window.draw(highlight);

	float rivetInset = 8.f;
	drawRivet(window, {pos.x + rivetInset, pos.y + rivetInset});
	drawRivet(window, {pos.x + size.x - rivetInset, pos.y + rivetInset});
	drawRivet(window, {pos.x + rivetInset, pos.y + size.y - rivetInset});
	drawRivet(window, {pos.x + size.x - rivetInset, pos.y + size.y - rivetInset});
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

	switch(bullet.state)
	{
	case BulletSlot::State::FILLED: {
		sf::RectangleShape casing({BULLET_WIDTH, BULLET_HEIGHT - 6.f});
		casing.setPosition({pos.x, pos.y + 6.f});
		casing.setFillColor(brassColor);
		window.draw(casing);

		sf::ConvexShape tip(3);
		tip.setPoint(0, {pos.x, pos.y + 6.f});
		tip.setPoint(1, {pos.x + BULLET_WIDTH, pos.y + 6.f});
		tip.setPoint(2, {pos.x + BULLET_WIDTH / 2.f, pos.y});
		tip.setFillColor(tipColor);
		window.draw(tip);
		break;
	}

	case BulletSlot::State::EMPTY:
		break;

	case BulletSlot::State::DEPLETING: {
		uint8_t alpha = static_cast<uint8_t>(255 * (1.f - bullet.animProgress));
		sf::Color fadingBrass = brassColor;
		fadingBrass.a = alpha;
		sf::Color fadingTip = tipColor;
		fadingTip.a = alpha;

		sf::RectangleShape casing({BULLET_WIDTH, BULLET_HEIGHT - 6.f});
		casing.setPosition({pos.x, pos.y + 6.f});
		casing.setFillColor(fadingBrass);
		window.draw(casing);

		sf::ConvexShape tip(3);
		tip.setPoint(0, {pos.x, pos.y + 6.f});
		tip.setPoint(1, {pos.x + BULLET_WIDTH, pos.y + 6.f});
		tip.setPoint(2, {pos.x + BULLET_WIDTH / 2.f, pos.y});
		tip.setFillColor(fadingTip);
		window.draw(tip);
		break;
	}

	case BulletSlot::State::REFILLING: {
		float fillHeight = (BULLET_HEIGHT - 6.f) * bullet.animProgress;

		if(fillHeight > 0.f)
		{
			sf::RectangleShape casing({BULLET_WIDTH, fillHeight});
			casing.setPosition({pos.x, pos.y + BULLET_HEIGHT - fillHeight});
			casing.setFillColor(brassColor);
			window.draw(casing);
		}

		if(bullet.animProgress > 0.85f)
		{
			float tipProgress = (bullet.animProgress - 0.85f) / 0.15f;
			sf::Color partialTip = tipColor;
			partialTip.a = static_cast<uint8_t>(255 * tipProgress);

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
	sf::CircleShape rivet(4.f);
	rivet.setOrigin({4.f, 4.f});
	rivet.setPosition(pos);
	rivet.setFillColor(GameConfig::UI::METAL_RIVET_COLOR);
	rivet.setOutlineColor(sf::Color(30, 32, 35));
	rivet.setOutlineThickness(1.f);
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
