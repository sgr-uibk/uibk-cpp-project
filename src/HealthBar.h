#pragma once
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/Transformable.hpp>

class HealthBar : public sf::Drawable, public sf::Transformable
{
  public:
	HealthBar(sf::Vector2f position, sf::Vector2f size, int maxHealth = 100);
	void setMaxHealth(int max);
	void setHealth(int health);
	void setPositionScreen(sf::Vector2f const &pos);
	void setFont(sf::Font const &font);
	void setTextVisible(bool visible);
	void setFillColors(sf::Color const &high, sf::Color const &mid, sf::Color const &low);
	int getHealth() const;
	int getMaxHealth() const;
	void triggerDamageFlash();

	void update(float dt);
	void healthCallback(int health, int max);

  private:
	void draw(sf::RenderTarget &target, sf::RenderStates states) const override;
	void updateVisuals();

	int m_maxHealth;
	int m_health;
	int m_midThreshold;
	int m_lowThreshold;
	sf::Color m_highColor;
	sf::Color m_midColor;
	sf::Color m_lowColor;

	sf::Vector2f m_size;
	sf::RectangleShape m_bg;
	sf::RectangleShape m_fill;
	sf::Font m_font;
	sf::Text m_text;

	float m_damageFlashTimer = 0.f;
	int m_lastHealth = 0;
};
