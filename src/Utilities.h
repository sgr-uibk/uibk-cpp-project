#pragma once
#include <spdlog/logger.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <stdexcept>
#include <SFML/Graphics.hpp>
#include <SFML/Audio/Music.hpp>
#include <ranges>
#include "Player/PlayerState.h"
#include "World/WorldState.h"
#include "GameConfig.h"

constexpr char DEFAULT_PATTERN[] = "[%L %T %P <%n> %s:%#\t] %^%v%$";

class ServerShutdownException : public std::runtime_error
{
  public:
	ServerShutdownException() : std::runtime_error("Server has shut down")
	{
	}
};

std::shared_ptr<spdlog::logger> createConsoleLogger(std::string const &name);
std::shared_ptr<spdlog::logger> createConsoleAndFileLogger(std::string const &name,
                                                           spdlog::level::level_enum logLevel = spdlog::level::debug);
inline sf::Vector2f cartesianToIso(sf::Vector2f cartesian)
{
	sf::Vector2f tile = cartesian / CARTESIAN_TILE_SIZE;

	float isoX = (tile.x - tile.y) * (TILE_WIDTH / 2.0f);
	float isoY = (tile.x + tile.y) * (TILE_HEIGHT / 2.0f);

	return {isoX, isoY};
}

inline sf::Vector2f isoToCartesian(sf::Vector2f iso)
{
	float tileX = (iso.x / (TILE_WIDTH / 2.0f) + iso.y / (TILE_HEIGHT / 2.0f)) / 2.0f;
	float tileY = (iso.y / (TILE_HEIGHT / 2.0f) - iso.x / (TILE_WIDTH / 2.0f)) / 2.0f;

	return sf::Vector2f(tileX, tileY) * CARTESIAN_TILE_SIZE;
}

struct RenderObject
{
	float sortY = 0.0f;
	sf::Drawable const *drawable = nullptr;
	std::optional<sf::Sprite> tempSprite;

	void draw(sf::RenderWindow &window) const
	{
		if(tempSprite.has_value())
			window.draw(*tempSprite);
		else if(drawable)
			window.draw(*drawable);
	}

	bool operator<(RenderObject const &other) const
	{
		return sortY < other.sortY;
	}
};

sf::Music &initMusic(std::string const &resourcePath, bool bStartPlaying = true);

template <size_t N, std::size_t... I>
constexpr std::array<InterpPlayerState, N> extractInterpStates(WorldState const &ws, std::index_sequence<I...>) noexcept
{
	return {ws.m_players[I].m_iState...};
}

template <size_t N, std::size_t... I>
constexpr std::array<InterpPlayerState, N> extractInterpStates(std::array<PlayerState, N> const &arr,
                                                               std::index_sequence<I...>) noexcept
{
	return {arr.m_iState};
}

template <std::size_t N>
constexpr std::array<InterpPlayerState, N> toInterpArrayConstexpr(WorldState const &ws) noexcept
{
	std::array<InterpPlayerState, N> const arr = extractInterpStates<N>(ws, std::make_index_sequence<N>{});
	return arr;
}

template <std::size_t N>
constexpr std::array<InterpPlayerState, N> toInterpArrayConstexpr(std::array<PlayerState, N> const &aps) noexcept
{
	std::array<InterpPlayerState, N> const arr = extractInterpStates<N>(aps, std::make_index_sequence<N>{});
	return arr;
}

template <std::ranges::range R, typename Proj> constexpr auto extract(R &r, Proj proj)
{
	auto tfd = std::views::transform(std::forward<R>(r), proj);
	return std::to_array(tfd);
}

template <std::ranges::sized_range R, typename Proj> constexpr auto extract2(R &r, Proj proj)
{
	using std::ranges::size;
	using Elem = std::remove_cvref_t<decltype(proj(*std::ranges::begin(r)))>;
	constexpr std::size_t N = size(r);

	std::array<Elem, N> out{};
	std::size_t i = 0;
	for(auto &&e : r)
	{
		out[i++] = proj(e);
	}
	return out;
}

template <std::size_t J> static PlayerState deserialize_one(sf::Packet &pkt)
{
	return PlayerState{static_cast<int>(J + 1), pkt};
}

template <std::size_t Sz, std::size_t... I>
std::array<PlayerState, Sz> deserializePlayerStateArray(sf::Packet &pkt, std::index_sequence<I...>) noexcept
{
	return {deserialize_one<I>(pkt)...};
}

template <std::size_t Sz, std::size_t... I>
void serializePlayerStateArray(sf::Packet &pkt, std::array<PlayerState, Sz> const &arr,
                               std::index_sequence<I...>) noexcept
{
	((arr[I].serialize(pkt)), ...); // fold over comma to evaluate each serialization in order
}
