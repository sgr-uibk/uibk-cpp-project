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
	Endpoint endpoint = {.ip = sf::IpAddress::Any, .port = 0};
	sf::TcpSocket tcpSocket;
};

class LobbyServer
{
  public:
	explicit LobbyServer(uint16_t tcpPort);
	~LobbyServer();
	void lobbyLoop(); // blocks until all players ready

	bool readyToStart() const;

	uint32_t findClient(Endpoint remote) const;
	WorldState startGame(int mapIndex);
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
