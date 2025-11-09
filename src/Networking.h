#pragma once
#include <cstdint>
#include <SFML/Network.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <spdlog/spdlog.h>

constexpr uint16_t PORT_TCP = 25106;
// It's a publicly registered, well-known port for multiplayer network traffic.
// https://learn.microsoft.com/en-us/gaming/gdk/docs/features/console/networking/game-mesh/preferred-local-udp-multiplayer-port-networking
constexpr uint16_t PORT_UDP = 3074;
constexpr uint32_t PROTOCOL_VERSION = 1;
constexpr float UNRELIABLE_TICK_RATE = 1.f / 20;
constexpr sf::Vector2u WINDOW_DIM{800, 600};
constexpr sf::Vector2f WINDOW_DIMf{WINDOW_DIM};
typedef uint32_t EntityId;

constexpr uint8_t MAX_PLAYERS = 4;
constexpr std::array ALL_PLAYER_COLORS{sf::Color::Red, sf::Color::Green, sf::Color::Yellow, sf::Color::Magenta};

static_assert(ALL_PLAYER_COLORS.size() >= MAX_PLAYERS,
              "Using more players than defined player colors. You should add the missing ones.");

// TODO: template this
constexpr auto PLAYER_COLORS = [] {
	std::array<sf::Color, MAX_PLAYERS> colors{};
	for(std::size_t i = 0; i < MAX_PLAYERS; ++i)
		colors[i] = ALL_PLAYER_COLORS[i];
	return colors;
}();

struct Endpoint // TODO, often this can be replaced by e.g. sf::TcpSocket.getRemote*()
{
	sf::IpAddress ip;
	uint16_t port;
};

enum class ReliablePktType : uint8_t
{
	JOIN_REQ = 1,
	JOIN_ACK,
	LOBBY_READY,
	LOBBY_UNREADY,
	LOBBY_UPDATE, // srv -> clients: broadcast lobby state updates
	START_GAME_REQUEST, // host requests to start game
	GAME_START,
	GAME_END,
	SERVER_SHUTDOWN,
	PLAYER_LEFT, // TODO
	_size
};

enum class UnreliablePktType : uint8_t
{
	MOVE = 1,
	SHOOT,
	SELECT_SLOT,
	USE_ITEM,
	SNAPSHOT,
	LAST
};

template <typename E> sf::Packet createPkt(E type)
{
	static_assert(std::is_enum_v<E>);
	sf::Packet pkt;
	pkt << static_cast<uint8_t>(type);
	return pkt;
}

template <typename E> void expectPkt(sf::Packet &pkt, E expectedType)
{
	static_assert(std::is_enum_v<E>);
	uint8_t actual;
	pkt >> actual;
	E actualEnum = static_cast<E>(actual);
	if(actual == 0 || expectedType != actualEnum)
	{
		std::string const message = "Unexpected packet type " + std::to_string(actual) + " (expected " +
		                            std::to_string(static_cast<uint8_t>(expectedType)) + ")";
		throw std::runtime_error(message);
	}
}

// Data is guaranteed to be fully sent/received even for non-blocking TCP sockets.
// When a partial transmission occurs, this function blocks until completion.
inline sf::Socket::Status checkedSend(sf::TcpSocket &sock, sf::Packet &pkt)
{
	sf::Socket::Status st;
	int i = 0;
	do
	{
		st = sock.send(pkt);
		i++;
	} while(st == sf::Socket::Status::Partial);
	if(i > 1)
		SPDLOG_INFO("Multipart pkt sent with {} parts", i);
	return st;
}

inline sf::Socket::Status checkedReceive(sf::TcpSocket &sock, sf::Packet &pkt)
{
	sf::Socket::Status st;
	int i = 0;
	do
	{
		st = sock.receive(pkt);
		i++;
	} while(st == sf::Socket::Status::Partial);
	if(i > 1)
		SPDLOG_INFO("Multipart pkt received with {} parts", i);
	return st;
}

// UDP is datagrams of a defined max size, no partial transfer handling needed.
inline sf::Socket::Status checkedSend(sf::UdpSocket &sock, sf::Packet &pkt, sf::IpAddress const destAddr,
                                      uint16_t const port)
{
	if(pkt.getDataSize() == 0)
	{
		SPDLOG_WARN("Trying to send empty packet");
		return sf::Socket::Status::Error;
	}
	sf::Socket::Status st = sock.send(pkt, destAddr, port);
	return st;
}

inline sf::Socket::Status checkedReceive(sf::UdpSocket &sock, sf::Packet &pkt,
                                         std::optional<sf::IpAddress> &destAddrOpt, uint16_t &port)
{
	sf::Socket::Status st = sock.receive(pkt, destAddrOpt, port);
	if(st != sf::Socket::Status::NotReady && pkt.getDataSize() == 0)
		SPDLOG_WARN("Received empty packet (status {})", (int)st);
	return st;
}

inline sf::Packet& operator<<(sf::Packet &pkt, const sf::Vector2f &vec)
{
	pkt << vec.x;
	pkt << vec.y;
	return pkt;
}

inline sf::Packet& operator>>(sf::Packet &pkt, sf::Vector2f &vec)
{
	pkt >> vec.x;
	pkt >> vec.y;
	return pkt;
}

inline sf::Packet& operator<<(sf::Packet &pkt, const sf::Angle &ang)
{
	pkt << ang.asRadians();
	return pkt;
}

inline sf::Packet& operator>>(sf::Packet &pkt, sf::Angle &ang)
{
	float radians;
	pkt >> radians;
	ang = sf::radians(radians);
	return pkt;
}

inline sf::Packet& operator<<(sf::Packet &pkt, sf::RectangleShape const &rec)
{
	pkt << rec.getPosition();
	pkt << rec.getSize();
	pkt << rec.getFillColor().toInteger();
	return pkt;
}

inline sf::Packet& operator>>(sf::Packet &pkt, sf::RectangleShape &rec)
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
