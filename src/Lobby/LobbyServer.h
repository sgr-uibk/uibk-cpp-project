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
    LobbyServer(uint16_t tcpPort, const std::shared_ptr<spdlog::logger>& logger);
    ~LobbyServer();
    void lobbyLoop(); // blocks until all players ready
    WorldState startGame(WorldState& worldState);
    void endGame(EntityId winner);

    std::vector<LobbyPlayer> m_slots; // Vector, clients join sequentially

private:
    void acceptNewClient();
    void handleClient(LobbyPlayer&);

    sf::TcpListener m_listener;
    sf::SocketSelector m_multiSock;
    uint32_t m_nextId = 1;
    std::shared_ptr<spdlog::logger> m_logger;
    uint8_t m_cReady;
};
