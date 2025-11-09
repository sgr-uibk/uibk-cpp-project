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
	explicit LobbyClient(const std::string &name, Endpoint lobbyServer = {sf::IpAddress::LocalHost, PORT_TCP});
	~LobbyClient();
	void bindGameSocket();
	void connect();
	void sendReady();
	void sendUnready();
	void sendStartGame();
	bool pollLobbyUpdate();
	std::array<PlayerState, MAX_PLAYERS> waitForGameStart();
	std::optional<std::array<PlayerState, MAX_PLAYERS>> checkForGameStart(); // Non-blocking check

	[[nodiscard]] bool getReadiness() const
	{
		return m_bReady;
	}

	[[nodiscard]] const std::vector<LobbyPlayerInfo> &getLobbyPlayers() const
	{
		return m_lobbyPlayers;
	}

	EntityId m_clientId;
	sf::TcpSocket m_lobbySock;
	std::string m_name;

	sf::UdpSocket m_gameSock;

  private:
	bool m_bReady;
	std::shared_ptr<spdlog::logger> m_logger;
	std::vector<LobbyPlayerInfo> m_lobbyPlayers;

	std::array<PlayerState, MAX_PLAYERS> parseGameStartPacket(sf::Packet &pkt);
};
