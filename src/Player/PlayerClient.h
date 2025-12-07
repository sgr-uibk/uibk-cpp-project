#pragma once
#include "PlayerState.h"
#include "Map/MapState.h"
#include <SFML/Graphics.hpp>
#include <functional>
#include <array>
#include <vector>

struct RenderObject;

class PlayerClient
{
	friend class InterpClient;

  public:
	using HealthCallback = std::function<void(int current, int max)>;

	PlayerClient(PlayerState &state, sf::Color const &color);
	PlayerClient(PlayerClient const &) = default;

	void update(float dt);
	void collectRenderObjects(std::vector<RenderObject> &queue) const;

	// apply authoritative server state (reconciliation)
	void applyServerState(PlayerState const &serverState);
	// input / local movement (prediction)
	void applyLocalMove(MapState const &map, sf::Vector2f delta);

	void setTurretRotation(sf::Angle angle);

	PlayerState const &getState() const
	{
		return m_state;
	}

	void registerHealthCallback(HealthCallback cb);
	void interp(InterpPlayerState const &s0, InterpPlayerState const &s1, float alpha);
	void overwriteInterpState(InterpPlayerState authState);

  private:
	void updateSprite();
	void updateNameText();
	void syncSpriteToState();
	static constexpr sf::Vector2f tankDimensions = {64, 64};
	PlayerState &m_state;

	// visuals
	sf::Color m_color;

	std::array<sf::Texture *, 8> m_hullTextures;
	std::array<sf::Texture *, 8> m_turretTextures;
	sf::Sprite m_hullSprite;
	sf::Sprite m_turretSprite;

	sf::Font &m_font; // must be declared before m_nameText
	sf::Text m_nameText;
	HealthCallback m_onHealthChanged;

	float m_shootAnimTimer;
	float m_lastShootCooldown;
	static constexpr float SHOOT_ANIM_DURATION = 0.1f;
};

template <size_t N> constexpr std::array<PlayerState, N> extractStates(std::array<PlayerClient, N> const &arr) noexcept
{
	return [&]<std::size_t... I>(std::index_sequence<I...>) noexcept {
		return std::array<PlayerState, N>{arr[I].getState()...};
	}(std::make_index_sequence<N>{});
}
