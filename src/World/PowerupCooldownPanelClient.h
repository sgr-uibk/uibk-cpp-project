#pragma once
#include "Player/PlayerState.h"
#include "PieChartShape.h"
#include <SFML/Graphics.hpp>
#include <array>

class PowerupCooldownPanel
{
  public:
	PowerupCooldownPanel(PlayerState const &playerState, sf::RenderWindow const &window);

	void update(float dt);
	void draw(sf::RenderWindow &window) const;

	void triggerCooldownFlash(PowerupType type);

  private:
	struct TypeSlotState
	{
		PowerupType type = PowerupType::NONE;
		enum class Phase
		{
			INACTIVE,
			ACTIVE_DURATION,
			REUSE_COOLDOWN
		} phase = Phase::INACTIVE;
		float totalDuration = 0.f;
		float remaining = 0.f;
		float flashTimer = 0.f;
	};

	void drawMetalPlateBackground(sf::RenderWindow &window, sf::Vector2f const &pos,
	                              sf::Vector2f const &size) const;
	void drawSlot(sf::RenderWindow &window, TypeSlotState const &slot,
	              sf::Vector2f const &pos) const;
	void drawRivet(sf::RenderWindow &window, sf::Vector2f const &pos) const;

	[[nodiscard]] sf::Vector2f calculatePanelPosition() const;
	[[nodiscard]] sf::Vector2f calculatePanelSize() const;
	[[nodiscard]] int countActiveSlots() const;

	PlayerState const &m_playerState;
	sf::RenderWindow const &m_window;

	std::array<TypeSlotState, PlayerState::NUM_POWERUP_TYPES> m_typeSlots;

	static constexpr float SLOT_SIZE = GameConfig::UI::POWERUP_PANEL_SLOT_SIZE;
	static constexpr float SLOT_SPACING = GameConfig::UI::POWERUP_PANEL_SLOT_SPACING;
	static constexpr float MARGIN = GameConfig::UI::POWERUP_PANEL_MARGIN;
	static constexpr float REUSE_COOLDOWN_DURATION = GameConfig::UI::POWERUP_REUSE_COOLDOWN;
	static constexpr float PANEL_PADDING = 12.f;
	static constexpr float FLASH_DURATION = 2.f;
};
