#include "Networking.h"
#include <SFML/Network.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <random>

int main(int argc, char **argv)
{
	std::string const playerName = (argc >= 2) ? argv[1] : "Unnamed client";
	uint16_t const port = (argc == 3) ? atoi(argv[2]) : PORT_TCP;

	auto logger = makeLogger(playerName);

	sf::IpAddress const localhost{0x7f'00'00'01};
	sf::TcpSocket sockLobby;
	if(sockLobby.connect(localhost, port) != sf::Socket::Status::Done)
	{
		SPDLOG_LOGGER_ERROR(logger, "Failed to connect to lobby");
		return 1;
	}
	sockLobby.setBlocking(true);

	sf::Packet pktJoinRequest;
	pktJoinRequest << PROTOCOL_VERSION << playerName;

	if(sf::Socket::Status status = sockLobby.send(pktJoinRequest); status != sf::Socket::Status::Done)
		SPDLOG_LOGGER_ERROR(logger, "Send JOIN_REQ returned Status {}", (int)status);

	// Block until getting JOIN_ACK
	sf::Packet pktJoinResponse;
	uint32_t clientId = 0;
	if(sockLobby.receive(pktJoinResponse) == sf::Socket::Status::Done)
	{
		uint8_t type;
		pktJoinResponse >> type;
		if(type == (uint8_t)ReliablePktType::JOIN_ACK)
		{
			pktJoinResponse >> clientId;
			SPDLOG_LOGGER_INFO(logger, "Joined lobby, got id {}", clientId);
		}
	}

	sf::sleep(sf::seconds(1)); // simulate wait

	sf::Packet pktLobbyReady;
	pktLobbyReady << (uint8_t)ReliablePktType::LOBBY_READY;
	pktLobbyReady << clientId;

	if(sf::Socket::Status status = sockLobby.send(pktLobbyReady); status != sf::Socket::Status::Done)
		SPDLOG_LOGGER_ERROR(logger, "Send LOBBY_READY returned Status {}", (int)status);

	SPDLOG_LOGGER_INFO(logger, "I'm ready.", clientId);

	// Wait for START_GAME
	while(true)
	{
		sf::Packet msg;
		if(sockLobby.receive(msg) == sf::Socket::Status::Done)
		{
			uint8_t type;
			msg >> type;
			if(type == (uint8_t)ReliablePktType::GAME_START)
			{
				SPDLOG_LOGGER_INFO(logger, "Received START_GAME!");
				sf::Vector2f spawnPoint;
				msg >> spawnPoint;
				float rad;
				msg >> rad;
				sf::Angle direction = sf::radians(rad);

				SPDLOG_LOGGER_INFO(logger, "My spawn point is ({},{}), direction angle = {}deg", spawnPoint.x,
				                   spawnPoint.y,
				                   direction.asDegrees());

				break;
			}
		}
	}

	SPDLOG_LOGGER_INFO(logger, "Game is starting!");

	// TODO tie in the unreliable protocol here, messages need to be sent over that.

	// simulate battle
	std::mt19937 mt(clientId);
	unsigned flips = 0;
	while(++flips)
	{
		uint64_t const randval = mt();
		if(randval % 2)
		{
			SPDLOG_LOGGER_INFO(logger, "Died after {} flips", flips);
			break;
		}
		sf::sleep(sf::milliseconds(100));
	}

	SPDLOG_LOGGER_INFO(logger, "Client shutting down.");

	return 0;
}
