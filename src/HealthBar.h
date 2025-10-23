#pragma once
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/Transformable.hpp>

class HealthBar : public sf::Drawable, public sf::Transformable
{
public:
	HealthBar(float width, float height, const sf::Font &font);

	void setMaxHealth(float max);
	void setHealth(float hp); // immediate set
	void setPositionScreen(const sf::Vector2f &pos);
	void setFont(const sf::Font &font);
	void setTextVisible(bool visible);
	void setFillColors(const sf::Color &high, const sf::Color &mid, const sf::Color &low,
	                   float midThreshold = 0.6f, float lowThreshold = 0.3f);
	float getHealth() const;
	float getMaxHealth() const;

	void update(float dt);

private:
	// Drawable override
	void draw(sf::RenderTarget &target, sf::RenderStates states) const override;

	void updateVisuals();

	float m_maxHealth;
	float m_health;

	sf::Vector2f m_size;
	sf::RectangleShape m_bg;
	sf::RectangleShape m_fill;
	sf::Text m_text;
	const sf::Font *m_font;

	// color ramp
	sf::Color m_highColor;
	sf::Color m_midColor;
	sf::Color m_lowColor;
	float m_midThreshold;
	float m_lowThreshold;
};
