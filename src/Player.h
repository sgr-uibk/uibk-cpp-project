#pragma once
#include <SFML/Graphics.hpp>
#include <functional>
#include <string>

class Map;

class Player
{
public:
	using HealthCallback = std::function<void(float current, float max)>;

	Player(const std::string &name, const sf::Color &color, Map &map, float maxHealth = 100.f);

	// gameplay
	void update(float dt);
	void draw(sf::RenderWindow &window) const;
	void movement(const sf::Vector2f &delta);
	void takeDamage(float amount); // also handles player dying
	void heal(float amount);
	void die();
	void revive(); // restore dead player to full health
	void updateSprite(sf::Texture const *tex = nullptr);

	// accessors
	float getHealth() const;
	float getMaxHealth() const;
	const std::string &getName() const;
	const sf::Sprite &getSprite() const;

	// UI callback
	void setHealthCallback(HealthCallback cb);

	// map
	void setPosition(const sf::Vector2f &pos);
	sf::Vector2f getPosition() const;

private:
	void setHealthInternal(float hp);

	std::string m_name;
	sf::Color m_color;
	float m_health;
	float m_maxHealth;
	Map const &m_map; // needed for checking wall collisions

	static constexpr float tankW = 64.f, tankH = 64.f;
	sf::Texture m_healthyTex;
	sf::Texture m_damagedTex;
	sf::Texture m_destroyedTex;
	sf::Sprite m_sprite; // sprite is constructed after textures

	HealthCallback m_onHealthChanged;
};
