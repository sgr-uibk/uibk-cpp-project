#pragma once
#include <vector>
#include <memory>
#include <SFML/Network.hpp>
#include <spdlog/logger.h>

#include "Lobby/LobbyServer.h"

class GameServer
{
  public:
	explicit GameServer(LobbyServer &lobbyServer, uint16_t gamePort, const std::shared_ptr<spdlog::logger> &logger);
	~GameServer();
	// Run the match loop (blocking).
	// Returns the player ids that participated.
	PlayerState *matchLoop();

	uint16_t m_gamePort;
	WorldState m_world;
	sf::Clock m_tickClock;
	size_t m_numAlive;

	void sendGameEnd(PlayerState* winner);
	void onPlayerKilled(uint32_t killerId, uint32_t victimId);

  private:
	void processPackets();
	void floodWorldState();
	void spawnItems();

	std::shared_ptr<spdlog::logger> m_logger;
	sf::UdpSocket m_gameSock;
	LobbyServer &m_lobby;
	sf::Clock m_itemSpawnClock;
};
