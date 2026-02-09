#pragma once
#include <SFML/Network.hpp>
#include "Lobby/LobbyClient.h"
#include "World/WorldClient.h"

// A thin facade for orchestrating Lobby/Battle loop
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
