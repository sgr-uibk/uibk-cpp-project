#pragma once
#include "RingBuffer.h"

#include <memory>
#include <SFML/Network.hpp>
#include <spdlog/logger.h>

#include "Lobby/LobbyClient.h"
#include "World/WorldClient.h"

class GameClient
{
  public:
	GameClient(WorldClient &world, LobbyClient &lobby, const std::shared_ptr<spdlog::logger> &logger);
	~GameClient();
	void update(sf::RenderWindow &window) const;
	void fetchFromServer();
	bool processReliablePackets(sf::TcpSocket &lobbySock) const;

  private:
	WorldClient &m_world;
	LobbyClient &m_lobby;
	Endpoint m_gameServer;
	RingQueue<std::pair<Tick, WorldState>, 8> m_snapshotBuffer;

	std::shared_ptr<spdlog::logger> m_logger;
};
