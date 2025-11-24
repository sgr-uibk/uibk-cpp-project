#pragma once
#include <memory>
#include <SFML/Network.hpp>
#include <spdlog/logger.h>

#include "Lobby/LobbyClient.h"
#include "World/WorldClient.h"

#include "Endscreen/Endscreen.h"

class GameClient
{
  public:
	GameClient(WorldClient &world, LobbyClient &lobby, const std::shared_ptr<spdlog::logger> &logger);
	~GameClient();
	void handleUserInputs(sf::RenderWindow &window);
	void syncFromServer() const;
	bool processReliablePackets(sf::TcpSocket &lobbySock, sf::RenderWindow &window);
	void render(sf::RenderWindow &window);

	bool m_showEndscreen = false;
	std::unique_ptr<Endscreen> m_endscreen;

  private:
	WorldClient &m_world;
	LobbyClient &m_lobby;
	Endpoint m_gameServer;

	sf::Clock m_endscreenClock;

	std::shared_ptr<spdlog::logger> m_logger;
};
