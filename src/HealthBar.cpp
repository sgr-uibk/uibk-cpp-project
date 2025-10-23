#include "HealthBar.h"
#include <algorithm>
#include <cmath>

HealthBar::HealthBar(const float width, const float height, const sf::Font &font)
	: m_maxHealth(100.f),
	  m_health(100.f),
	  m_size(width, height),
	  m_text(font),
	  m_font(nullptr),
	  m_highColor(80, 200, 60),
	  m_midColor(240, 200, 64),
	  m_lowColor(220, 60, 60),
	  m_midThreshold(0.6f),
	  m_lowThreshold(0.3f)
{
	// Appearance inherited from Shape
	m_bg.setSize(m_size);
	m_bg.setFillColor(sf::Color(50, 50, 50, 200));
	m_bg.setOutlineColor(sf::Color::Black);
	m_bg.setOutlineThickness(2.f);
	m_fill.setSize(m_size);
	m_fill.setFillColor(m_highColor);

	// Numeric health / max health display
	m_font = &font;
	m_text.setFont(font);
	m_text.setCharacterSize(static_cast<unsigned int>(m_size.y * 0.7f));
	m_text.setFillColor(sf::Color::White);
	m_text.setStyle(sf::Text::Bold);
}

void HealthBar::setMaxHealth(const float max)
{
	m_maxHealth = std::max(1.f, max); // for sanity
	m_health = std::min(m_health, m_maxHealth);
	updateVisuals();
}

void HealthBar::setHealth(const float hp)
{
	m_health = std::clamp(hp, 0.f, m_maxHealth);
	updateVisuals();
}

void HealthBar::setPositionScreen(const sf::Vector2f &pos)
{
	setPosition(pos);
	m_bg.setPosition(pos);
	m_fill.setPosition(pos);
	// move it inwards a few pixels
	m_text.setPosition({pos.x + 6.f, pos.y + (m_size.y - m_text.getCharacterSize()) / 2.f - 2.f});
}

void HealthBar::setFont(const sf::Font &font)
{
	m_font = &font;
	m_text.setFont(font);
	setPositionScreen(getPosition());
}

void HealthBar::setTextVisible(const bool visible)
{
	sf::Color c = m_text.getFillColor();
	c.a = visible ? 255 : 0; // use alpha channel to hide if invisible
	m_text.setFillColor(c);
}

void HealthBar::setFillColors(const sf::Color &high, const sf::Color &mid, const sf::Color &low,
                              const float midThreshold, const float lowThreshold)
{
	m_highColor = high;
	m_midColor = mid;
	m_lowColor = low;
	m_midThreshold = midThreshold;
	m_lowThreshold = lowThreshold;
	updateVisuals();
}

float HealthBar::getHealth() const
{
	return m_health;
}

float HealthBar::getMaxHealth() const
{
	return m_maxHealth;
}

void HealthBar::update(float const dt)
{
	updateVisuals();
}

void HealthBar::draw(sf::RenderTarget &target, sf::RenderStates states) const
{
	states.transform *= getTransform();
	target.draw(m_bg, states);
	target.draw(m_fill, states);
	if(m_font)
		target.draw(m_text, states);
}

void HealthBar::updateVisuals()
{
	float ratio = 0.f;
	if(m_maxHealth > 0.f)
		ratio = std::clamp(m_health / m_maxHealth, 0.f, 1.f);
	// set length and color of the "remaining" health rectangle
	m_fill.setSize({m_size.x * ratio, m_size.y});

	if(ratio > m_midThreshold)
		m_fill.setFillColor(m_highColor);
	else if(ratio > m_lowThreshold)
		m_fill.setFillColor(m_midColor);
	else
		m_fill.setFillColor(m_lowColor);

	if(m_font)
	{
		const int shownHealth = static_cast<int>(std::round(m_health));
		const int shownMaxHealth = static_cast<int>(std::round(m_maxHealth));
		m_text.setString(std::to_string(shownHealth) + " / " + std::to_string(shownMaxHealth));
	}
}
