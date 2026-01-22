#pragma once
#include <memory>
#include <SFML/Network.hpp>
#include <spdlog/logger.h>

#include "Lobby/LobbyServer.h"

class GameServer
{
  public:
	GameServer(LobbyServer &lobbyServer, uint16_t gamePort, WorldState const &wsInit);
	~GameServer();
	PlayerState *matchLoop();

	bool tickStep();
	PlayerState* m_winner = nullptr;
	PlayerState* winner() const;
	void forceEnd();

	uint16_t m_gamePort;
	WorldState m_world;
	sf::Clock m_tickClock;
	Tick m_authTick = 0;
	std::array<int32_t, MAX_PLAYERS> m_lastClientTicks = {0};

  private:
	void processPackets();
	void floodWorldState();
	void spawnItems();

	sf::UdpSocket m_gameSock;
	LobbyServer &m_lobby;
	sf::Clock m_itemSpawnClock;
	size_t m_nextItemSpawnIndex;

	bool m_forceEnd = false;
};
