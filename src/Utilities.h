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
	std::optional<sf::Sprite> tempSprite = std::nullopt;

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

template <typename T, std::size_t Sz, std::size_t... I>
std::array<T, Sz> deserializeToArray(sf::Packet &pkt, std::index_sequence<I...>) noexcept
{
	return {T{static_cast<int>(I + 1), pkt}...};
}

template <typename T, std::size_t Sz, std::size_t... I>
void serializeFromArray(sf::Packet &pkt, std::array<T, Sz> const &arr, std::index_sequence<I...>) noexcept
{
	((arr[I].serialize(pkt)), ...); // fold over comma to evaluate each serialization in order
}

template <typename T, std::size_t Sz, std::size_t... I>
constexpr std::array<T, Sz> fill(T const &v, std::index_sequence<I...>)
{
	return std::array<T, Sz>{{(static_cast<void>(I), v)...}}; // copy Sz times
}
