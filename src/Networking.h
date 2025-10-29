#pragma once
#include <cstdint>
#include <SFML/Network.hpp>
#include <SFML/Graphics/RectangleShape.hpp>

constexpr uint16_t PORT_TCP = 25106;
// It's a publicly registered, well-known port for multiplayer network traffic.
// https://learn.microsoft.com/en-us/gaming/gdk/docs/features/console/networking/game-mesh/preferred-local-udp-multiplayer-port-networking
constexpr uint16_t PORT_UDP = 3074;
constexpr uint32_t PROTOCOL_VERSION = 1;
constexpr float UNRELIABLE_TICK_RATE = 1.f / 20;
typedef uint32_t EntityId;


constexpr uint8_t MAX_PLAYERS = 4;
constexpr std::array ALL_PLAYER_COLORS{
	sf::Color::Red, sf::Color::Green, sf::Color::Yellow, sf::Color::Magenta
};
static_assert(ALL_PLAYER_COLORS.size() >= MAX_PLAYERS,
              "Using more players defined player colors. You should add the missing ones.");

constexpr auto PLAYER_COLORS = [] {
	std::array<sf::Color, MAX_PLAYERS> colors{};
	for(std::size_t i = 0; i < MAX_PLAYERS; ++i)
		colors[i] = ALL_PLAYER_COLORS[i];
	return colors;
}();

enum class ReliablePktType : uint8_t
{
	JOIN_REQ = 1,
	JOIN_ACK,
	LOBBY_READY, // client -> srv
	LOBBY_UPDATE, // TODO srv -> clients
	GAME_START, // srv -> clients
	GAME_END, // TODO
	PLAYER_LEFT, // TODO
};

enum class UnreliablePktType : uint8_t
{
	MOVE = 1
};

inline sf::Packet operator<<(sf::Packet &pkt, const sf::Vector2f &vec)
{
	pkt << vec.x;
	pkt << vec.y;
	return pkt;
}

inline sf::Packet operator>>(sf::Packet &pkt, sf::Vector2f &vec)
{
	pkt >> vec.x;
	pkt >> vec.y;
	return pkt;
}

inline sf::Packet operator<<(sf::Packet &pkt, const sf::Angle &ang)
{
	pkt << ang.asRadians();
	return pkt;
}

inline sf::Packet operator>>(sf::Packet &pkt, sf::Angle &ang)
{
	float radians;
	pkt >> radians;
	ang = sf::radians(radians);
	return pkt;
}

inline sf::Packet operator<<(sf::Packet &pkt, sf::RectangleShape const &rec)
{
	pkt << rec.getPosition();
	pkt << rec.getSize();
	pkt << rec.getFillColor().toInteger();
	return pkt;
}

inline sf::Packet operator>>(sf::Packet &pkt, sf::RectangleShape &rec)
{
	uint32_t colorInt = 0;
	sf::Vector2f pos, sz;
	pkt >> pos;
	pkt >> sz;
	pkt >> colorInt;
	rec.setSize(sz);
	rec.setPosition(pos);
	rec.setFillColor(sf::Color(colorInt));
	return pkt;
}
