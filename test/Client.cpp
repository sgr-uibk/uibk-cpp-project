#include <SFML/Network.hpp>
#include <SFML/System.hpp>
#include <SFML/Graphics.hpp>
#include <thread>
#include <spdlog/spdlog.h>

#include "Networking.h"
#include "Utilities.h"
#include "Game/GameClient.h"
#include "Lobby/LobbyClient.h"
#include "World/WorldClient.h"

int main(int argc, char **argv)
{
	auto logger = createConsoleAndFileLogger("Client");
	std::string playerName = (argc >= 2) ? argv[1] : "Unnamed";
	sf::IpAddress serverAddr = (argc >= 2) ? sf::IpAddress(atoi(argv[2])) : sf::IpAddress::LocalHost;
	uint16_t const lobbyPort = (argc >= 3) ? atoi(argv[3]) : PORT_TCP;
	uint16_t gamePort = (argc >= 4) ? atoi(argv[4]) : PORT_UDP;
	SPDLOG_LOGGER_INFO(spdlog::get("Client"), "Starting Client. TCP port {}, UDP port {}, name {}", lobbyPort, gamePort,
	                   playerName);

	LobbyClient lobbyClient(playerName, Endpoint{.ip = serverAddr, .port = lobbyPort});
	lobbyClient.connect();

	sf::RenderWindow window(sf::VideoMode(WINDOW_DIM), "TankGame Client", sf::Style::Resize);
	window.setFramerateLimit(60);

	while(window.isOpen())
	{
		sf::sleep(sf::seconds(1));
		lobbyClient.sendReady();
		SPDLOG_LOGGER_INFO(spdlog::get("Client"), "I'm ready, waiting for GAME_START...");

		sf::Clock timeout;
		sf::Time const maxWaitTime = sf::seconds(300);
		LobbyEvent event = LobbyEvent::NONE;

		while(timeout.getElapsedTime() < maxWaitTime)
		{
			event = lobbyClient.pollLobbyUpdate();
			if(event == LobbyEvent::GAME_STARTED)
			{
				break;
			}
			else if(event == LobbyEvent::SERVER_DISCONNECTED)
			{
				SPDLOG_CRITICAL("Server disconnected while waiting for game start");
				return EXIT_FAILURE;
			}
			sf::sleep(sf::milliseconds(100));
		}

		if(event != LobbyEvent::GAME_STARTED)
		{
			SPDLOG_CRITICAL("Timed out waiting for game start");
			return EXIT_FAILURE;
		}

		auto playerStates = lobbyClient.getGameStartData();
		WorldClient worldClient(window, lobbyClient.m_clientId, playerStates);
		lobbyClient.m_lobbySock.setBlocking(false); // make reliable channel pollable
		GameClient gameClient(worldClient, lobbyClient);

		// Battle loop
		while(window.isOpen())
		{
			gameClient.update(window);
			gameClient.processUnreliablePackets();
			if(gameClient.processReliablePackets(lobbyClient.m_lobbySock))
				break; // game ended
		}
	}
}
