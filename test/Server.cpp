#include <iostream>
#include <string>
#include <SFML/Network.hpp>
#include <thread>
#include <cstdlib>
#include <spdlog/spdlog.h>
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
	std::atomic<bool> bForceEndGame{false};
	int nextMapIndex = -1;

	std::thread cmdThread([&] {
		while(true)
		{
			std::string cmd;
			std::getline(std::cin, cmd);

			if(cmd == "quit")
				exit(EXIT_SUCCESS);
			if(cmd == "end")
			{
				bForceEndGame = true;
				SPDLOG_LOGGER_INFO(spdlog::get("Server"), "The game end was forced!");
			}
			else if(cmd.rfind("map ", 0) == 0)
			{
				std::string mapStr = cmd.substr(4);
				try
				{
					int idx = std::stoi(mapStr);
					if(idx < 0 || idx >= (int)Maps::MAP_PATHS.size())
					{
						SPDLOG_LOGGER_WARN(spdlog::get("Server"), "Invalid map index {}, using default", idx);
					}
					else
					{
						nextMapIndex = idx;
						SPDLOG_LOGGER_INFO(spdlog::get("Server"), "Next game map set to {}", nextMapIndex);
					}
				}
				catch(...)
				{
					SPDLOG_LOGGER_WARN(spdlog::get("Server"), "Invalid map index: {}", mapStr);
				}
			}
			else
				SPDLOG_LOGGER_INFO(spdlog::get("Server"), "Unknown command: {}", cmd);
		}
	});

	while(cmdThread.joinable())
	{
		lobbyServer.lobbyLoop();

		SPDLOG_LOGGER_INFO(spdlog::get("Server"), "All {} players ready. Starting game...", MAX_PLAYERS);
		auto wsInit = lobbyServer.startGame(nextMapIndex);
		nextMapIndex = -1; // next map will be random again
		GameServer gameServer(lobbyServer, udpPort, wsInit);
		SPDLOG_LOGGER_INFO(spdlog::get("Server"), "Game started. Switching to UDP loop.");
		if(PlayerState *winningPlayer = gameServer.matchLoop(bForceEndGame))
		{
			lobbyServer.endGame(winningPlayer->m_id);
			SPDLOG_LOGGER_INFO(spdlog::get("Server"), "Game Ended, winner {}. returning to lobby", winningPlayer->m_id);
		}
		else
			lobbyServer.endGame(0);
		bForceEndGame = false;
	}

	SPDLOG_LOGGER_INFO(spdlog::get("Server"), "Server shutting down.");
	return 0;
}
