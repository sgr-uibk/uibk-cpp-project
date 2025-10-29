#include <SFML/Network.hpp>
#include <SFML/System.hpp>
#include <SFML/Graphics.hpp>
#include <optional>
#include <thread>
#include <spdlog/spdlog.h>

#include "Networking.h"
#include "Utilities.h"
#include "World/WorldClient.h"
#include "World/WorldState.h"

int main(int argc, char **argv)
{
	auto logger = createConsoleAndFileLogger("Client");
	std::string playerName = (argc >= 2) ? argv[1] : "Unnamed";
	uint16_t const tcpPort = (argc >= 2) ? atoi(argv[1]) : PORT_TCP;
	uint16_t udpPort = (argc >= 3) ? atoi(argv[2]) : PORT_UDP;
	SPDLOG_LOGGER_INFO(logger, "Starting Client. TCP port {}, UDP port {}, name {}",
	                   tcpPort, udpPort, playerName);

	sf::TcpSocket lobbySock;
	if(lobbySock.connect(sf::IpAddress::LocalHost, tcpPort) != sf::Socket::Status::Done)
	{
		SPDLOG_LOGGER_ERROR(logger, "Failed to connect to lobby TCP");
		return 1;
	}
	lobbySock.setBlocking(true);

	sf::UdpSocket battleSock;
	auto status = sf::Socket::Status::NotReady;
	for(size_t i = udpPort; i < udpPort + MAX_PLAYERS + 1; ++i)
	{
		status = battleSock.bind(i);
		if(status == sf::Socket::Status::Done)
		{
			udpPort = i;
			SPDLOG_LOGGER_INFO(logger, "UDP Bind on port {} succeeded.", udpPort);
			break;
		}
		battleSock.unbind();
	}
	if(status != sf::Socket::Status::Done)
	{
		SPDLOG_LOGGER_ERROR(logger, "Failed to bind to any UDP port [{}, {}]",
		                    udpPort, udpPort + MAX_PLAYERS);
		return 1;
	}
	battleSock.setBlocking(false);

	sf::Packet joinReqPkt;
	joinReqPkt << PROTOCOL_VERSION << playerName << udpPort;
	if(lobbySock.send(joinReqPkt) != sf::Socket::Status::Done)
	{
		SPDLOG_LOGGER_ERROR(logger, "Failed to send JOIN_REQ");
		return 1;
	}

	sf::Packet joinAckPkt;
	EntityId clientId = 0;
	sf::Socket::Status error = lobbySock.receive(joinAckPkt);
	if(error == sf::Socket::Status::Done)
	{
		uint8_t type;
		joinAckPkt >> type;
		if(type == uint8_t(ReliablePktType::JOIN_ACK))
		{
			joinAckPkt >> clientId;
			SPDLOG_LOGGER_INFO(logger, "Joined lobby, got id {}", clientId);
		}
	}
	else
	{
		SPDLOG_LOGGER_ERROR(logger, "Failed to receive JOIN_ACK: {}", int(error));
#ifdef SFML_SYSTEM_LINUX
		perror("Error from OS");
#endif
		return 1;
	}
	assert(clientId != 0);

	sf::Packet readyPkt;
	readyPkt << uint8_t(ReliablePktType::LOBBY_READY);
	readyPkt << clientId;
	if(lobbySock.send(readyPkt) != sf::Socket::Status::Done)
	{
		SPDLOG_LOGGER_ERROR(logger, "Failed to send LOBBY_READY");
		return 1;
	}
	std::array<PlayerState, MAX_PLAYERS> pss;
	SPDLOG_LOGGER_INFO(logger, "I'm ready, waiting for GAME_START...");
	while(true)
	{
		sf::Packet startPkt;
		if(lobbySock.receive(startPkt) != sf::Socket::Status::Done)
		{
			SPDLOG_LOGGER_ERROR(logger, "Failed to receive GAME_START");
			return 1;
		}
		uint8_t type;
		startPkt >> type;
		if(type == uint8_t(ReliablePktType::GAME_START))
		{
			size_t numPlayers;
			startPkt >> numPlayers;
			SPDLOG_LOGGER_INFO(logger, "Received GAME_START for {} players", numPlayers);
			for(unsigned i = 0; i < numPlayers; ++i)
			{
				sf::Vector2f pos;
				startPkt >> pos;
				sf::Angle rot;
				startPkt >> rot;
				pss[i] = PlayerState(i + 1, pos, rot);
				SPDLOG_LOGGER_INFO(logger, "My spawn point is ({},{}), direction angle = {}deg",
				                   pos.x, pos.y, rot.asDegrees());
			}
			break;
		}
		sf::sleep(sf::milliseconds(100));
	}
	sf::Vector2u constexpr WINDOW_DIM{800, 600};
	sf::RenderWindow window(sf::VideoMode(WINDOW_DIM), "TankGame Client", sf::Style::Resize);
	window.setFramerateLimit(60);
	WorldClient worldClient(window, clientId, pss);
	sf::IpAddress serverAddr = sf::IpAddress::LocalHost;

	// Battle loop
	lobbySock.setBlocking(false); // make reliable channel pollable
	while(window.isOpen())
	{
		worldClient.pollInputs();

		sf::Packet outPkt = worldClient.update();
		worldClient.draw(window);
		window.display();

		if(outPkt.getDataSize() > 0)
		{
			// WorldClient.update() produces tick-rate-limited packets, so now is the right time to send.
			if(battleSock.send(outPkt, serverAddr, PORT_UDP) != sf::Socket::Status::Done)
				SPDLOG_LOGGER_ERROR(logger, "UDP send failed");
		}

		sf::Packet snapPkt;
		std::optional<sf::IpAddress> srcAddrOpt;
		uint16_t srcPort;
		if(battleSock.receive(snapPkt, srcAddrOpt, srcPort) == sf::Socket::Status::Done)
		{
			WorldState snapshot{sf::Vector2f(WINDOW_DIM)};
			snapshot.deserialize(snapPkt);
			worldClient.applyServerSnapshot(snapshot);
			auto ops = worldClient.getOwnPlayerState();
			SPDLOG_LOGGER_INFO(logger, "Applied snapshot: ({:.0f},{:.0f}), {:.0f}°, hp={}",
				   ops.m_pos.x, ops.m_pos.y, ops.m_rot.asDegrees(), ops.m_health);
		}

		sf::Packet reliablePkt;
		if(lobbySock.receive(reliablePkt) != sf::Socket::Status::Done)
		{
			uint8_t type;
			reliablePkt >> type;
			EntityId winnerId;
			reliablePkt >> winnerId;
			if(type == uint8_t(ReliablePktType::GAME_END))
			{
				// TODO save the player names somewhere, so that we cant print the winner here
				SPDLOG_LOGGER_INFO(logger, "Battle is over, winner id {}, returning to lobby.",
				                   winnerId);
				break;
			}
		}
	}

	// TODO: loop back to lobby
	lobbySock.disconnect();
	battleSock.unbind();
	SPDLOG_LOGGER_INFO(logger, "Client shutting down.");
	return 0;
}
