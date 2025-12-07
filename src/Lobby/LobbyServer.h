#pragma once
#include "../Utilities.h"
#include <SFML/Network.hpp>
#include "World/WorldState.h"

struct LobbyPlayer
{
	EntityId id = 0;
	std::string name = "Invalid";
	bool bValid = false;
	bool bReady = false;
	sf::IpAddress udpAddr = sf::IpAddress{0};
	uint16_t gamePort = 0;
	sf::TcpSocket tcpSocket;
};

class LobbyServer
{
  public:
	explicit LobbyServer(uint16_t tcpPort);
	~LobbyServer();
	void lobbyLoop(); // blocks until all players ready
	WorldState startGame();
	void deduplicatePlayerName(std::string &name) const;
	void endGame(EntityId winner);

	std::array<LobbyPlayer, MAX_PLAYERS> m_slots;

  private:
	void acceptNewClient();
	void handleClient(LobbyPlayer &);
	void broadcastLobbyUpdate();

	sf::TcpListener m_listener;
	sf::SocketSelector m_multiSock;
	uint8_t m_cReady = 0;
	bool m_gameStartRequested = false;
};
