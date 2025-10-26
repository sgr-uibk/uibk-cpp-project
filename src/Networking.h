#pragma once
#include <cstdint>
#include <SFML/Network.hpp>

constexpr uint16_t PORT_TCP = 25101;
constexpr uint16_t PORT_UDP = 25101;
constexpr uint8_t MAX_PLAYERS = 4;
constexpr uint32_t PROTOCOL_VERSION = 1;


enum class ReliablePktType : uint8_t
{
	JOIN_REQ = 1,
	JOIN_ACK,
	LOBBY_READY, // client -> srv
	LOBBY_UPDATE, // TODO srv -> clients
	GAME_START,  // srv -> clients
	GAME_END, // TODO
	PLAYER_LEFT, // TODO
};

enum class UnreliablePktType : uint8_t
{
	INPUT = 1,
	SNAPSHOT,
	UDP_HELLO
};

inline sf::Packet operator<<(sf::Packet &lhs, const sf::Vector2f &rhs)
{
	lhs << rhs.x;
	lhs << rhs.y;
	return lhs;
}

inline sf::Packet operator>>(sf::Packet & lhs, sf::Vector2f & rhs)
{
	lhs >> rhs.x;
	lhs >> rhs.y;
	return lhs;
}

//#pragma pack(push,1)
struct UdpInputPacket
{
	uint8_t packetType; // = (uint8_t)UdpPacketType::INPUT
	uint32_t clientId;
	uint32_t seq; // monotonic
	uint16_t inputMask; // bitflags for movement/fire/
	int16_t aimAngle; // fixed angle (deg * 100)
};

struct UdpSnapshotHeader
{
	uint8_t packetType; // = (uint8_t)UdpPacketType::SNAPSHOT
	uint32_t serverSeq;
	uint8_t playerCount;
};

//#pragma pack(pop)
