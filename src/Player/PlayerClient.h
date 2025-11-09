#pragma once
#include "PlayerState.h"
#include <Map/MapState.h>
#include <SFML/Graphics.hpp>
#include <functional>

class PlayerClient
{
  public:
	using HealthCallback = std::function<void(int current, int max)>;

	PlayerClient(PlayerState &state, const sf::Color &color);
	PlayerClient(const PlayerClient &) = default;

	void update(float dt);
	void draw(sf::RenderWindow &window) const;

	// apply authoritative server state (reconciliation)
	void applyServerState(const PlayerState &serverState);
	// input / local movement (prediction)
	void applyLocalMove(MapState const &map, sf::Vector2f delta);

	const PlayerState &getState() const
	{
		return m_state;
	}

	void registerHealthCallback(HealthCallback cb);

  private:
	void updateSprite();
	void updateNameText();
	static constexpr sf::Vector2f tankDimensions = {64, 64};
	PlayerState &m_state;

	// visuals
	sf::Color m_color;
	sf::Texture &m_healthyTex;
	sf::Texture &m_damagedTex;
	sf::Texture &m_deadTex;
	sf::Sprite m_sprite;
	sf::Font &m_font; // must be declared before m_nameText
	sf::Text m_nameText;
	HealthCallback m_onHealthChanged;

	void syncSpriteToState();
};
