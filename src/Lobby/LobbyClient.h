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

enum class LobbyEvent
{
	NONE,
	LOBBY_UPDATED,
	GAME_STARTED,
	SERVER_DISCONNECTED
};

class LobbyClient
{
  public:
	explicit LobbyClient(std::string const &name, Endpoint lobbyServer = {sf::IpAddress::LocalHost, PORT_TCP});
	~LobbyClient();
	void bindGameSocket();
	void connect();
	void sendReady();
	LobbyEvent pollLobbyUpdate();

	[[nodiscard]] bool getReadiness() const
	{
		return m_bReady;
	}

	[[nodiscard]] std::vector<LobbyPlayerInfo> const &getLobbyPlayers() const
	{
		return m_lobbyPlayers;
	}

	[[nodiscard]] std::array<PlayerState, MAX_PLAYERS> getGameStartData() const
	{
		return m_startData;
	}

	EntityId m_clientId;
	sf::TcpSocket m_lobbySock;
	std::string m_name;

	sf::UdpSocket m_gameSock;

  private:
	bool m_bReady;
	std::string m_loggerName; // Store original logger name for proper cleanup
	std::vector<LobbyPlayerInfo> m_lobbyPlayers;
	std::array<PlayerState, MAX_PLAYERS> m_startData;

	void parseGameStartPacket(sf::Packet &pkt);
};