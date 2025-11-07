#pragma once
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/Transformable.hpp>
#include "Player.h"

class HealthBar : public sf::Drawable, public sf::Transformable
{
  public:
	HealthBar(sf::Vector2f position, sf::Vector2f size, int maxHealth = 100);
	void setMaxHealth(int max);
	void setHealth(int health);
	void setPositionScreen(const sf::Vector2f &pos);
	void setFont(const sf::Font &font);
	void setTextVisible(bool visible);
	void setFillColors(const sf::Color &high, const sf::Color &mid, const sf::Color &low);
	int getHealth() const;
	int getMaxHealth() const;

	void update(float dt);
	void healthCallback(int health, int max);

  private:
	// Drawable override
	void draw(sf::RenderTarget &target, sf::RenderStates states) const override;
	void updateVisuals();

	int m_maxHealth;
	int m_health;
	// color ramp
	int m_midThreshold;
	int m_lowThreshold;
	sf::Color m_highColor;
	sf::Color m_midColor;
	sf::Color m_lowColor;

	sf::Vector2f m_size;
	sf::RectangleShape m_bg;
	sf::RectangleShape m_fill;
	sf::Text m_text;
	sf::Font m_font;
};