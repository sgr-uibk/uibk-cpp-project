#pragma once
#include "../Utilities.h"
#include <SFML/Network.hpp>
#include "World/WorldState.h"

struct LobbyPlayer
{
	uint32_t id{0};
	std::string name;
	bool bValid{false};
	bool bReady{false};
	sf::IpAddress udpAddr;
	uint16_t gamePort;
	sf::TcpSocket tcpSocket;
};

class LobbyServer
{
  public:
	explicit LobbyServer(uint16_t tcpPort);
	~LobbyServer();
	void lobbyLoop(); // blocks until all players ready
	void startGame(WorldState &worldState);
	void deduplicatePlayerName(std::string &name) const;
	void endGame(EntityId winner);

	std::vector<LobbyPlayer> m_slots; // Vector, clients join sequentially

  private:
	void acceptNewClient();
	void handleClient(LobbyPlayer &);
	void broadcastLobbyUpdate();

	sf::TcpListener m_listener;
	sf::SocketSelector m_multiSock;
	uint32_t m_tentativeId = 1;
	uint8_t m_cReady = 0;
	bool m_gameStartRequested = false;
};
