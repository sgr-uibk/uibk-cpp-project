#pragma once
#include "../Networking.h"
#include <SFML/Network.hpp>
#include <memory>
#include <spdlog/spdlog.h>
#include <spdlog/logger.h>

#include "Player/PlayerState.h"

class LobbyClient
{
public:
	explicit LobbyClient(const std::string &name,
	                     Endpoint lobbyServer = {sf::IpAddress::LocalHost, PORT_TCP});
	~LobbyClient();
	void bindGameSocket();
	void connect();
	void sendReady();
	std::array<PlayerState, MAX_PLAYERS> waitForGameStart();

	[[nodiscard]] bool getReadiness() const
	{
		return m_bReady;
	}


	EntityId m_clientId;
	sf::TcpSocket m_lobbySock;
	std::string m_name;

	sf::UdpSocket m_gameSock;

private:
	bool m_bReady; // once set, cannot be unset
	std::shared_ptr<spdlog::logger> m_logger;
};
