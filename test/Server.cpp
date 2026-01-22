#include <iostream>
#include <string>
#include <SFML/Network.hpp>
#include <SFML/System.hpp>
#include <vector>
#include <thread>
#include <cstdlib>
#include <spdlog/spdlog.h>
#include <atomic>

#include "Networking.h"
#include "Utilities.h"
#include "Lobby/LobbyServer.h"
#include "Game/GameServer.h"

int main(int argc, char **argv)
{
	auto logger = createConsoleAndFileLogger("Server");
	uint16_t const tcpPort = (argc >= 2) ? atoi(argv[1]) : PORT_TCP;
	uint16_t const udpPort = (argc >= 3) ? atoi(argv[2]) : PORT_UDP;
	SPDLOG_LOGGER_INFO(spdlog::get("Server"), "Starting Server. TCP port {}, UDP port {}", tcpPort, udpPort);

	LobbyServer lobbyServer(tcpPort);
	std::unique_ptr<GameServer> gameServer;

	std::atomic<bool> running{true};

	std::thread cmdThread([&]{
        while(running)
        {
            std::string cmd;
            std::getline(std::cin, cmd);

            if(cmd == "quit")
                running = false;
			else if(cmd == "end" && gameServer)
			{
				gameServer->forceEnd();
				SPDLOG_LOGGER_INFO(spdlog::get("Server"), "The game end was forced!");
			}
            else
                SPDLOG_LOGGER_INFO(spdlog::get("Server"), "Unknown command: {}", cmd);
        }
    });

	while(running)
	{

		if (!gameServer)
        {
            lobbyServer.tickStep();

            if (lobbyServer.readyToStart())
            {
				SPDLOG_LOGGER_INFO(spdlog::get("Server"), "All {} players ready. Starting game...", MAX_PLAYERS);
                auto wsInit = lobbyServer.startGame();
                gameServer = std::make_unique<GameServer>(lobbyServer, udpPort, wsInit);
                SPDLOG_LOGGER_INFO(spdlog::get("Server"), "Game started. Switching to UDP loop.");
            }
        }

		if (gameServer)
        {
            bool gameEnded = gameServer->tickStep();
            if (gameEnded)
            {
                auto winner = gameServer->winner();
                lobbyServer.endGame(winner ? winner->m_id : 0);
                gameServer.reset();
				SPDLOG_LOGGER_INFO(spdlog::get("Server"), 
                   "Game Ended, winner {}. returning to lobby", 
                   winner ? winner->m_id : 0);
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	cmdThread.join();
	SPDLOG_LOGGER_INFO(spdlog::get("Server"), "Server shutting down.");
	return 0;
}
