#pragma once
#include "Player/PlayerState.h"
#include <SFML/Graphics.hpp>
#include <array>

class AmmunitionDisplay
{
  public:
	AmmunitionDisplay(PlayerState const &playerState, sf::RenderWindow const &window);

	void update(float dt);
	void draw(sf::RenderWindow &window) const;
	void onShoot();
	[[nodiscard]] bool hasAmmo() const;
	void triggerEmptyFlash();

  private:
	struct BulletSlot
	{
		enum class State
		{
			FILLED,
			DEPLETING,
			EMPTY,
			REFILLING
		} state = State::FILLED;
		float animProgress = 0.f;
	};

	void drawMetalPlateBackground(sf::RenderWindow &window, sf::Vector2f const &pos,
	                              sf::Vector2f const &size) const;
	void drawBullet(sf::RenderWindow &window, size_t index, sf::Vector2f const &pos) const;
	void drawRivet(sf::RenderWindow &window, sf::Vector2f const &pos) const;

	[[nodiscard]] sf::Vector2f calculatePanelPosition() const;
	[[nodiscard]] sf::Vector2f calculatePanelSize() const;

	PlayerState const &m_playerState;
	sf::RenderWindow const &m_window;

	std::array<BulletSlot, GameConfig::UI::AMMO_BULLET_COUNT> m_bullets;
	float m_reloadProgress = 0.f;
	int m_currentReloadBullet = -1;

	float m_prevCooldownRemaining = 0.f;
	float m_flashTimer = 0.f;

	static constexpr float BULLET_WIDTH = GameConfig::UI::AMMO_BULLET_WIDTH;
	static constexpr float BULLET_HEIGHT = GameConfig::UI::AMMO_BULLET_HEIGHT;
	static constexpr float BULLET_SPACING = GameConfig::UI::AMMO_BULLET_SPACING;
	static constexpr float MARGIN = GameConfig::UI::AMMO_PANEL_MARGIN;
	static constexpr int BULLET_COUNT = GameConfig::UI::AMMO_BULLET_COUNT;
	static constexpr float PANEL_PADDING = 12.f;
	static constexpr float DEPLETING_ANIM_DURATION = 0.1f;
	static constexpr float FLASH_DURATION = 0.5f;
};
