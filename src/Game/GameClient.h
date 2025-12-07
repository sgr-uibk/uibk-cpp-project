#pragma once
#include "RingBuffer.h"

#include <memory>
#include <SFML/Network.hpp>
#include <spdlog/logger.h>

#include "Lobby/LobbyClient.h"
#include "World/WorldClient.h"

// A thin facade for orchestrating Lobby/Battle loop
// TODO I don't see a real responsibility, this class could be inlined.
class GameClient
{
  public:
	GameClient(WorldClient &world, LobbyClient &lobby);
	~GameClient();
	void update(sf::RenderWindow &window) const;
	void processUnreliablePackets();
	bool processReliablePackets(sf::TcpSocket &lobbySock) const;

  private:
	WorldClient &m_world;
	LobbyClient &m_lobby;
	Endpoint m_gameServer;
};
