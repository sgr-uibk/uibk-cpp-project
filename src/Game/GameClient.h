#pragma once
#include <memory>
#include <SFML/Network.hpp>
#include <spdlog/logger.h>

#include "Lobby/LobbyClient.h"
#include "World/WorldClient.h"

class GameClient
{
public:
	GameClient(WorldClient &world, LobbyClient &lobby,
	           const std::shared_ptr<spdlog::logger> &logger);
	~GameClient();
	void handleUserInputs(sf::RenderWindow &window) const;
	void syncFromServer() const;
	bool processReliablePackets(sf::TcpSocket &lobbySock) const;

private:
	WorldClient &m_world;
	LobbyClient &m_lobby;
	Endpoint m_gameServer;

	std::shared_ptr<spdlog::logger> m_logger;
};
