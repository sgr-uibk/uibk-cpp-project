#pragma once
#include "../Utilities.h"
#include <SFML/Network.hpp>
#include "World/WorldState.h"
#include <atomic>

struct LobbyPlayer
{
	uint32_t id{0};
	std::string name;
	bool bValid{false};
	bool bReady{false};
	sf::IpAddress udpAddr = sf::IpAddress::Any;
	uint16_t gamePort{0};
	sf::TcpSocket tcpSocket;

	int totalKills{0};
	int totalDeaths{0};
};

class LobbyServer
{
  public:
	explicit LobbyServer(uint16_t tcpPort);
	~LobbyServer();
	void lobbyLoop(); // blocks until all players ready
	void stop()
	{
		m_running = false;
	}
	bool isRunning() const
	{
		return m_running;
	}

	void startGame(WorldState &worldState);
	void deduplicatePlayerName(std::string &name) const;
	void endGame(EntityId winner);
	void resetLobbyState();
	void updatePlayerStats(std::array<PlayerState, MAX_PLAYERS> const &playerStates);

	std::vector<LobbyPlayer> m_slots; // Vector, clients join sequentially

  private:
	void acceptNewClient();
	void handleClient(LobbyPlayer &);
	void broadcastLobbyUpdate();

	sf::TcpListener m_listener;
	sf::SocketSelector m_multiSock;
	uint32_t m_nextId = 1;
	uint8_t m_cReady = 0;
	bool m_gameInProgress = false;
	std::atomic<bool> m_running{true};
};
