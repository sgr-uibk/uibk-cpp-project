#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>

class WallState
{
  public:
	WallState();
	WallState(float x, float y, float w, float h, int maxHealth = 100);

	void takeDamage(int amount);
	void repair(int amount);
	void destroy();
	void setHealth(int health);

	[[nodiscard]] bool isDestroyed() const;
	[[nodiscard]] int getHealth() const;
	[[nodiscard]] int getMaxHealth() const;
	[[nodiscard]] float getHealthPercent() const;
	[[nodiscard]] sf::FloatRect getGlobalBounds() const;
	[[nodiscard]] const sf::RectangleShape &getShape() const;
	[[nodiscard]] sf::RectangleShape &getShape();

	void updateVisuals();

	void serialize(sf::Packet &pkt) const;
	void deserialize(sf::Packet &pkt);

  private:
	sf::RectangleShape m_shape;
	int m_health;
	int m_maxHealth;
	bool m_isDestroyed;

	static constexpr int DEFAULT_WALL_HEALTH = 100;
};
