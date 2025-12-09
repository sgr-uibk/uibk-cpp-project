#include <SFML/Network.hpp>
#include <SFML/System.hpp>
#include <vector>
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

	while(true)
	{
		lobbyServer.lobbyLoop();

		SPDLOG_LOGGER_INFO(spdlog::get("Server"), "All {} players ready. Starting game...", MAX_PLAYERS);

		// Initialize world with map
		std::array<PlayerState, MAX_PLAYERS> emptyPlayers{};
		WorldState initialWorld = WorldState::fromTiledMap("map/arena.json", emptyPlayers);
		if(initialWorld.getMap().getSpawns().empty())
		{
			SPDLOG_LOGGER_WARN(spdlog::get("Server"), "No spawn points found in map! Using default world.");
		}

		lobbyServer.startGame(initialWorld);
		GameServer gameServer(lobbyServer, udpPort, initialWorld);
		SPDLOG_LOGGER_INFO(spdlog::get("Server"), "Game started. Switching to UDP loop.");
		PlayerState *winningPlayer = gameServer.matchLoop();

		lobbyServer.updatePlayerStats(gameServer.m_world.getPlayers());

		if(winningPlayer)
			lobbyServer.endGame(winningPlayer->m_id);
		else
			lobbyServer.endGame(0);
		SPDLOG_LOGGER_INFO(spdlog::get("Server"), "Game Ended, winner {}. returning to lobby", winningPlayer->m_id);
	}
	return 0;
}
