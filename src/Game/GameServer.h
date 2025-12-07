#pragma once
#include <vector>
#include <memory>
#include <SFML/Network.hpp>
#include <spdlog/logger.h>

#include "Lobby/LobbyServer.h"

class GameServer
{
  public:
	explicit GameServer(LobbyServer &lobbyServer, uint16_t gamePort);
	~GameServer();
	PlayerState *matchLoop();

	uint16_t m_gamePort;
	WorldState m_world;
	sf::Clock m_tickClock;
	Tick m_authTick = 0;
	std::array<int32_t, MAX_PLAYERS> m_lastClientTicks = {0};

  private:
	void processPackets();
	void floodWorldState();
	void spawnItems();
	void checkPlayerConnections();

	sf::UdpSocket m_gameSock;
	LobbyServer &m_lobby;
	sf::Clock m_itemSpawnClock;
	size_t m_nextItemSpawnIndex;
};
