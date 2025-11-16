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
	PlayerState *matchLoop();

	uint16_t m_gamePort;
	WorldState m_world;
	sf::Clock m_tickClock;
	size_t m_numAlive;
	Tick m_authTick = 0;

  private:
	void processPackets();
	void floodWorldState();

	std::shared_ptr<spdlog::logger> m_logger;
	sf::UdpSocket m_gameSock;
	LobbyServer &m_lobby;
};
