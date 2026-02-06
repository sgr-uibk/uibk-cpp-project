#include "HealthBar.h"
#include "ResourceManager.h"
#include "GameConfig.h"
#include <algorithm>

namespace
{
constexpr float FLASH_DURATION = 0.3f;
} // namespace

HealthBar::HealthBar(sf::Vector2f position, sf::Vector2f size, int maxHealth)
	: m_maxHealth(maxHealth), m_health(maxHealth),
	  m_midThreshold(static_cast<int>(maxHealth * GameConfig::UI::HEALTH_HIGH_THRESHOLD)),
	  m_lowThreshold(static_cast<int>(maxHealth * GameConfig::UI::HEALTH_LOW_THRESHOLD)), m_highColor(80, 200, 60),
	  m_midColor(240, 200, 64), m_lowColor(220, 60, 60), m_size(size),
	  m_font(ResourceManager<sf::Font>::inst().load("Font/LiberationSans-Regular.ttf")), m_text(m_font)
{
	m_bg.setSize(m_size);
	m_bg.setFillColor(sf::Color(50, 50, 50, 200));
	m_bg.setOutlineColor(sf::Color::Black);
	m_bg.setOutlineThickness(2.f);
	m_fill.setSize(m_size);
	m_fill.setFillColor(m_highColor);

	m_text.setCharacterSize(static_cast<unsigned int>(m_size.y * 0.7f));
	m_text.setFillColor(sf::Color::White);
	m_text.setStyle(sf::Text::Bold);
	setPositionScreen(position);
}

void HealthBar::setMaxHealth(int const max)
{
	m_maxHealth = max;
	m_midThreshold = static_cast<int>(max * GameConfig::UI::HEALTH_HIGH_THRESHOLD);
	m_lowThreshold = static_cast<int>(max * GameConfig::UI::HEALTH_LOW_THRESHOLD);
	m_health = std::min(m_health, m_maxHealth);
	updateVisuals();
}

void HealthBar::setHealth(int const health)
{
	m_health = health;
	updateVisuals();
}

void HealthBar::setPositionScreen(sf::Vector2f const &pos)
{
	setPosition(pos);
	m_bg.setPosition({0.f, 0.f});
	m_fill.setPosition({0.f, 0.f});
	m_text.setPosition({6.f, (m_size.y - m_text.getCharacterSize()) / 2.f - 2.f});
}

void HealthBar::setFont(sf::Font const &font)
{
	m_font = font;
	m_text.setFont(font);
	setPositionScreen(getPosition());
}

void HealthBar::setTextVisible(bool const visible)
{
	sf::Color c = m_text.getFillColor();
	c.a = visible ? 255 : 0;
	m_text.setFillColor(c);
}

void HealthBar::setFillColors(sf::Color const &high, sf::Color const &mid, sf::Color const &low)
{
	m_highColor = high;
	m_midColor = mid;
	m_lowColor = low;
}

int HealthBar::getHealth() const
{
	return m_health;
}

int HealthBar::getMaxHealth() const
{
	return m_maxHealth;
}

void HealthBar::triggerDamageFlash()
{
	m_damageFlashTimer = FLASH_DURATION;
}

void HealthBar::update(float const dt)
{
	if(m_health < m_lastHealth)
		triggerDamageFlash();
	m_lastHealth = m_health;

	if(m_damageFlashTimer > 0.f)
		m_damageFlashTimer -= dt;

	updateVisuals();
}

void HealthBar::healthCallback(int const health, int const max)
{
	setMaxHealth(max);
	setHealth(health);
}

void HealthBar::draw(sf::RenderTarget &target, sf::RenderStates states) const
{
	states.transform *= getTransform();
	target.draw(m_bg, states);
	target.draw(m_fill, states);

	if(m_damageFlashTimer > 0.f)
	{
		sf::RectangleShape flash(m_size);
		float const alpha = (m_damageFlashTimer / FLASH_DURATION) * 150.f;
		flash.setFillColor(sf::Color(255, 0, 0, static_cast<std::uint8_t>(alpha)));
		target.draw(flash, states);
	}

	target.draw(m_text, states);
}

void HealthBar::updateVisuals()
{
	assert(m_maxHealth > 0);

	float const healthFraction = static_cast<float>(m_health) / static_cast<float>(m_maxHealth);
	m_fill.setSize({m_size.x * healthFraction, m_size.y});

	if(m_health > m_midThreshold)
		m_fill.setFillColor(m_highColor);
	else if(m_health > m_lowThreshold)
		m_fill.setFillColor(m_midColor);
	else
		m_fill.setFillColor(m_lowColor);

	m_text.setString(std::to_string(m_health) + " / " + std::to_string(m_maxHealth));
}
