#pragma once
#include <spdlog/logger.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <stdexcept>
#include "Networking.h"

constexpr char DEFAULT_PATTERN[] = "[%L %T %P <%n> %s:%#\t] %^%v%$";

class ServerShutdownException : public std::runtime_error
{
  public:
	ServerShutdownException() : std::runtime_error("Server has shut down")
	{
	}
};

enum class GameEndResult
{
	RETURN_TO_LOBBY,
	RETURN_TO_MAIN_MENU
};

std::shared_ptr<spdlog::logger> createConsoleLogger(std::string const &name);
std::shared_ptr<spdlog::logger> createConsoleAndFileLogger(std::string const &name,
                                                           spdlog::level::level_enum logLevel = spdlog::level::debug);

constexpr float TILE_WIDTH = 64.0f;
constexpr float TILE_HEIGHT = 32.0f;
constexpr float CARTESIAN_TILE_SIZE = TILE_HEIGHT;

inline sf::Vector2f cartesianToIso(sf::Vector2f cartesian)
{
	float tileX = cartesian.x / CARTESIAN_TILE_SIZE;
	float tileY = cartesian.y / CARTESIAN_TILE_SIZE;

	float isoX = (tileX - tileY) * (TILE_WIDTH / 2.0f);
	float isoY = (tileX + tileY) * (TILE_HEIGHT / 2.0f);

	return {isoX, isoY};
}

inline sf::Vector2f isoToCartesian(sf::Vector2f iso)
{
	float tileX = (iso.x / (TILE_WIDTH / 2.0f) + iso.y / (TILE_HEIGHT / 2.0f)) / 2.0f;
	float tileY = (iso.y / (TILE_HEIGHT / 2.0f) - iso.x / (TILE_WIDTH / 2.0f)) / 2.0f;

	float cartX = tileX * CARTESIAN_TILE_SIZE;
	float cartY = tileY * CARTESIAN_TILE_SIZE;

	return {cartX, cartY};
}