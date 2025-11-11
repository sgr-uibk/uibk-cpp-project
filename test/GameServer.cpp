// server.cpp
#include <thread>
#include <chrono>
#include <iostream>
#include <SFML/Network.hpp>
#include <spdlog/spdlog.h>

#include "World/WorldState.h"
#include "Player/PlayerState.h"
#include "Utilities.h"
#include "Networking.h"

static constexpr unsigned short SERVER_PORT = 54000u;
static constexpr unsigned short CLIENT_PORT = 54001u;
static constexpr sf::Vector2f WINDOW_DIM{800, 600};

int main()
{
	std::shared_ptr<spdlog::logger> const logger = createConsoleLogger("Unreliable Server");
	WorldState world(WINDOW_DIM);

	PlayerState serverP(1, {100.f, 100.f}, 100);
	world.setPlayer(serverP);

	sf::UdpSocket socket;
	if(socket.bind(SERVER_PORT) != sf::Socket::Status::Done)
	{
		SPDLOG_LOGGER_ERROR(logger, "bind on port {} failed", SERVER_PORT);
		return 1;
	}
	socket.setBlocking(false);

	sf::IpAddress clientAddr = sf::IpAddress::LocalHost;
	unsigned short clientPort = CLIENT_PORT;

	sf::Clock tickClk;
	while(true)
	{
		sf::Packet rxPkt;
		std::optional<sf::IpAddress> srcAddrOpt;
		unsigned short srcPort;
		if(socket.receive(rxPkt, srcAddrOpt, srcPort) == sf::Socket::Status::Done)
		{
			uint8_t type;
			rxPkt >> type;
			if(static_cast<UnreliablePktType>(type) == UnreliablePktType::MOVE)
			{
				EntityId playerId;
				rxPkt >> playerId;
				sf::Vector2f posDelta;
				rxPkt >> posDelta;
				sf::Angle rot;
				rxPkt >> rot;
				if(!world.getPlayers().empty())
				{
					PlayerState &ps = world.getPlayers()[0];
					ps.moveOn(world.map(), posDelta);
					SPDLOG_LOGGER_INFO(logger, "player: pos=({},{}), rotDeg={}, h={}", ps.m_pos.x, ps.m_pos.y,
					                   ps.m_rot.asDegrees(), ps.m_health);
				}
			}
			else if(static_cast<UnreliablePktType>(type) == UnreliablePktType::SPECTATOR_MOVE)
			{
				EntityId spectatorId;
				sf::Vector2f posDelta;
				sf::Angle aimAngle;
				rxPkt >> spectatorId;
				rxPkt >> posDelta;
				rxPkt >> aimAngle;

				SPDLOG_LOGGER_DEBUG(logger, "Ignoring SPECTATOR_MOVE for player {}", spectatorId);
			}
			// remember this client
			clientAddr = srcAddrOpt.value();
			clientPort = srcPort;
		}

		// fixed-timestep world update
		float dt = tickClk.restart().asSeconds();
		if(dt < UNRELIABLE_TICK_RATE)
			std::this_thread::sleep_for(std::chrono::duration<float>(UNRELIABLE_TICK_RATE - dt));
		world.update(UNRELIABLE_TICK_RATE);

		// broadcast snapshot to clients
		sf::Packet snap;
		world.serialize(snap);
		if(socket.send(snap, clientAddr, clientPort) != sf::Socket::Status::Done)
			SPDLOG_LOGGER_ERROR(logger, "Failed to broadcast snapshot!");
	}

	return 0;
}
