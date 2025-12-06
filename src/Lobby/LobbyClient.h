#pragma once
#include "../Networking.h"
#include <SFML/Network.hpp>
#include <memory>
#include <spdlog/spdlog.h>
#include <spdlog/logger.h>

#include "Player/PlayerState.h"

struct LobbyPlayerInfo
{
	uint32_t id;
	std::string name;
	bool bReady;
};

class LobbyClient
{
  public:
	explicit LobbyClient(std::string const &name, Endpoint lobbyServer = {sf::IpAddress::LocalHost, PORT_TCP});
	~LobbyClient();
	void bindGameSocket();
	void connect();
	void sendReady();
	bool pollLobbyUpdate();
	std::optional<std::array<PlayerState, MAX_PLAYERS>> waitForGameStart(sf::Time timeout);

	[[nodiscard]] bool getReadiness() const
	{
		return m_bReady;
	}

	[[nodiscard]] std::vector<LobbyPlayerInfo> const &getLobbyPlayers() const
	{
		return m_lobbyPlayers;
	}

	EntityId m_clientId;
	sf::TcpSocket m_lobbySock;
	std::string m_name;

	sf::UdpSocket m_gameSock;

  private:
	bool m_bReady;
	std::vector<LobbyPlayerInfo> m_lobbyPlayers;
};
