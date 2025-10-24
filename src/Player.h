#pragma once
#include <SFML/Graphics.hpp>
#include <functional>
#include <string>

#include "Map.h"

class Player
{
public:
	using HealthCallback = std::function<void(int current, int max)>;

	Player(std::string name, const sf::Color &color, Map &map, int maxHealth = 100);

	// gameplay
	void update(float dt);
	void draw(sf::RenderWindow &window) const;
	void movement(const sf::Vector2f &delta);
	void takeDamage(int amount);
	void heal(int amount);
	void die();
	void revive(); // restore dead player to full health
	void updateSprite(sf::Texture const *tex = nullptr);

	// accessors
	int getHealth() const;
	int getMaxHealth() const;
	const std::string &getName() const;
	const sf::Sprite &getSprite() const;

	// UI callback
	void setHealthCallback(HealthCallback cb);

	// map
	void setPosition(const sf::Vector2f &pos);
	sf::Vector2f getPosition() const;

private:
	void setHealthInternal(int health);

	std::string m_name;
	sf::Color m_color;
	int m_maxHealth;
	int m_health;
	int m_midThreshold;
	int m_lowThreshold;
	Map const &m_map; // needed for checking wall collisions

	static constexpr float tankW = 64.f, tankH = 64.f;
	sf::Texture m_healthyTex;
	sf::Texture m_damagedTex;
	sf::Texture m_deadTex;
	sf::Sprite m_sprite; // sprite is constructed after textures

	HealthCallback m_onHealthChanged;
};
