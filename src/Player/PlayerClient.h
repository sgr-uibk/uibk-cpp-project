#pragma once
#include "PlayerState.h"
#include "TankSpriteManager.h"
#include "HealthBar.h"
#include "Map/MapState.h"
#include <SFML/Graphics.hpp>
#include <functional>
#include <array>
#include <vector>
#include <SFML/Audio/Sound.hpp>
#include <SFML/Audio/SoundBuffer.hpp>

struct RenderObject;

class PlayerClient
{
	friend class InterpClient;

  public:
	using HealthCallback = std::function<void(int current, int max)>;

	PlayerClient(PlayerState &state, sf::Color const &color);
	PlayerClient(PlayerClient const &) = default;

	void update(float dt);
	void collectRenderObjects(std::vector<RenderObject> &queue, EntityId ownPlayerId = 0) const;
	void drawSilhouette(sf::RenderWindow &window, std::uint8_t alpha = 100) const;

	void applyServerState(PlayerState const &serverState);
	void applyLocalMove(MapState const &map, sf::Vector2f delta);

	void setCannonRotation(sf::Angle angle);

	PlayerState const &getState() const
	{
		return m_state;
	}

	void registerHealthCallback(HealthCallback cb);
	void interp(InterpPlayerState const &s0, InterpPlayerState const &s1, float alpha);
	void overwriteInterpState(InterpPlayerState authState);
	void playShotSound();

  private:
	void updateNameText();
	void syncSpriteToState();
	static constexpr sf::Vector2f tankDimensions = {64, 64};
	PlayerState &m_state;

	sf::Color m_color;

	sf::Sprite m_hullSprite;
	sf::Sprite m_turretSprite;
	TankSpriteManager::TankState m_currentTankState;

	sf::Font &m_font;
	sf::Text m_nameText;

	sf::SoundBuffer &m_shootSoundBuf;
	sf::Sound m_shootSound;
	HealthCallback m_onHealthChanged;
	mutable HealthBar m_healthBar;

	float m_shootAnimTimer = 0.f;
	float m_lastShootCooldown = 0.f;
	static constexpr float SHOOT_ANIM_DURATION = 0.1f;
};

template <size_t N> constexpr std::array<PlayerState, N> extractStates(std::array<PlayerClient, N> const &arr) noexcept
{
	return [&]<std::size_t... I>(std::index_sequence<I...>) noexcept {
		return std::array<PlayerState, N>{arr[I].getState()...};
	}(std::make_index_sequence<N>{});
}
