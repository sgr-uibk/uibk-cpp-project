#include <SFML/Network.hpp>
#include <SFML/System.hpp>
#include <array>
#include <vector>
#include <optional>
#include <thread>
#include <cstdlib>
#include <spdlog/spdlog.h>

#include "Networking.h"
#include "Utilities.h"
#include "World/WorldState.h"

struct LobbyPlayer
{
	uint32_t id{0};
	std::string name;
	bool bValid{false};
	bool bReady{false};
	sf::IpAddress udpAddr;
	uint16_t udpPort;
	sf::TcpSocket tcpSocket;
};

sf::Vector2f WINDOW_DIM{800, 600};


int main(int argc, char **argv)
{
	auto logger = createConsoleAndFileLogger("Server");
	uint16_t const tcpPort = (argc >= 2) ? atoi(argv[1]) : PORT_TCP;
	uint16_t const udpPort = (argc >= 3) ? atoi(argv[2]) : PORT_UDP;
	SPDLOG_LOGGER_INFO(logger, "Starting Server. TCP port {}, UDP port {}", tcpPort, udpPort);

	// TCP lobby listener
	sf::TcpListener listener;
	if(listener.listen(tcpPort) != sf::Socket::Status::Done)
	{
		SPDLOG_LOGGER_ERROR(logger, "Failed to listen TCP port {}", tcpPort);
		return 1;
	}
	listener.setBlocking(false);
	sf::SocketSelector multiSock;
	multiSock.add(listener);

	sf::UdpSocket battleSock;
	sf::Socket::Status status = battleSock.bind(udpPort);
	if(status != sf::Socket::Status::Done)
	{
		SPDLOG_LOGGER_ERROR(logger, "Failed to bind UDP port {}: {}", udpPort, int(status));
		return 1;
	}
	battleSock.setBlocking(false);

	std::vector<LobbyPlayer> lobbyPlayers;
	lobbyPlayers.reserve(4);
	uint32_t nextClientId = 1;

	// Lobby loop: accept joins, receive ready flags
	while(true)
	{
		if(multiSock.wait(sf::milliseconds(250)))
		{
			// Got a message. Is it a new client ?
			if(multiSock.isReady(listener))
			{
				sf::TcpSocket newClientSock;
				if(listener.accept(newClientSock) == sf::Socket::Status::Done)
				{
					assert(newClientSock.getRemoteAddress().has_value()); // Know exists from status done.
					sf::IpAddress const remoteAddr = newClientSock.getRemoteAddress().value();
					LobbyPlayer p{.id = nextClientId, .udpAddr = remoteAddr, .tcpSocket = std::move(newClientSock)};
					p.tcpSocket.setBlocking(false);

					sf::Packet joinReqPkt;
					if(p.tcpSocket.receive(joinReqPkt) != sf::Socket::Status::Done)
					{
						p.tcpSocket.disconnect();
						continue;
					}
					uint32_t protoVer;
					joinReqPkt >> protoVer;
					if(protoVer != PROTOCOL_VERSION)
					{
						SPDLOG_LOGGER_ERROR(logger, "Protocol mismatch: {} != {}", PROTOCOL_VERSION, protoVer);
						p.tcpSocket.disconnect();
						continue;
					}
					joinReqPkt >> p.name;
					joinReqPkt >> p.udpPort;
					SPDLOG_LOGGER_INFO(logger, "Client id {}, udpPrt {} ", p.id, p.udpPort);

					sf::Packet joinAckPkt;
					joinAckPkt << uint8_t(ReliablePktType::JOIN_ACK);
					joinAckPkt << p.id;
					SPDLOG_LOGGER_INFO(logger, "This client gets id: {}. ", p.id);
					if(p.tcpSocket.send(joinAckPkt) != sf::Socket::Status::Done)
						SPDLOG_LOGGER_ERROR(logger, "Failed to send JOIN_ACK to {} (id {})", p.name, p.id);

					// All good from server side. Make the client valid.
					p.bValid = true;
					nextClientId++;
					multiSock.add(p.tcpSocket);
					SPDLOG_LOGGER_INFO(logger, "Player {} (id {}) joined lobby", p.name, p.id);
					lobbyPlayers.push_back(std::move(p));
				}
			}

			// Do the known clients have something to say ?
			size_t readyCount = 0;
			for(auto &p : lobbyPlayers)
			{
				if(!p.bValid)
					continue;
				auto &sock = p.tcpSocket;
				if(multiSock.isReady(sock))
				{
					sf::Packet rxPkt;
					auto status = sock.receive(rxPkt);
					if(status == sf::Socket::Status::Disconnected)
					{
						SPDLOG_LOGGER_INFO(logger, "Player {} (id {}) disconnected", p.name, p.id);
						multiSock.remove(sock);
						p.bValid = false;
						continue;
					}
					if(status == sf::Socket::Status::Done)
					{
						uint8_t type;
						rxPkt >> type;
						if(type == uint8_t(ReliablePktType::LOBBY_READY))
						{
							uint32_t clientId;
							rxPkt >> clientId;
							assert(p.id == clientId);
							p.bReady = true;
							SPDLOG_LOGGER_INFO(logger, "Player {} (id {}) ready", p.name, p.id);
						}
					}
				}
				if(p.bReady)
					++readyCount;
			}

			if(readyCount == MAX_PLAYERS)
			{
				SPDLOG_LOGGER_INFO(logger, "All {} players ready. Starting game...", readyCount);
				break;
			}
		}
		sf::sleep(sf::milliseconds(100));
	}

	srand(0); // deterministic for reproducibility
	WorldState world(WINDOW_DIM);
	for(auto &p : lobbyPlayers)
	{
		if(!p.bValid)
			continue;
		// every player gets a random spawn point,
		// TODO: in future choose from a set of possible spawn points (map feature)
		sf::Vector2f spawn = {float(rand()) / float(RAND_MAX / WINDOW_DIM.x),
		                      float(rand()) / float(RAND_MAX / WINDOW_DIM.y)};
		sf::Angle rot = sf::degrees(float(rand()) / float(RAND_MAX / 360));
		PlayerState ps(p.id, spawn);
		ps.m_rot = rot;
		world.setPlayer(ps);
	}

	sf::Packet startPkt;
	auto const &playerStates = world.getPlayers();
	startPkt << uint8_t(ReliablePktType::GAME_START);
	startPkt << playerStates.size();
	for(size_t i = 0; i < playerStates.size(); ++i)
	{
		startPkt << playerStates[i].getPosition();
		startPkt << playerStates[i].getRotation();
	}
	// Distribute spawn points with GAME_START pkt
	for(auto &p : lobbyPlayers)
	{
		if(!p.bValid)
			continue;
		if(p.tcpSocket.send(startPkt) != sf::Socket::Status::Done)
			SPDLOG_LOGGER_ERROR(logger, "Failed to send GAME_START to {} (id {})", p.name, p.id);
	}

	SPDLOG_LOGGER_INFO(logger, "Game started. Switching to UDP loop.");
	sf::Clock tickClock;
	size_t numAlive = world.getPlayers().size();
	while(numAlive > 1)
	{
		sf::Packet rxPkt;
		std::optional<sf::IpAddress> srcAddrOpt;
		uint16_t srcPort;
		if(battleSock.receive(rxPkt, srcAddrOpt, srcPort) == sf::Socket::Status::Done)
		{
			uint8_t type;
			rxPkt >> type;
			switch(UnreliablePktType(type))
			{
			case UnreliablePktType::MOVE: {
				uint32_t clientId;
				sf::Vector2f posDelta;
				sf::Angle rot;
				rxPkt >> clientId;
				rxPkt >> posDelta;
				rxPkt >> rot; // TODO the server does not care about it - ok to remove ?
				auto &players = world.getPlayers();
				if(clientId <= players.size() && lobbyPlayers[clientId - 1].bValid)
				{
					PlayerState &ps = world.getPlayerById(clientId);

					ps.moveOn(world.getMap(), posDelta);
					SPDLOG_LOGGER_INFO(logger, "Recv MOVE id {} pos=({},{}), rotDeg={}", clientId, ps.m_pos.x,
					                   ps.m_pos.y, ps.m_rot.asDegrees());
				}
				else
					SPDLOG_LOGGER_INFO(logger, "Dropping packet from id {}", clientId);
				break;
			}
			default:
				SPDLOG_LOGGER_INFO(logger, "Unhandled unreliable packet type: {}.", type);
			}

			// fix tick rate
			float dt = tickClock.restart().asSeconds();
			if(dt < UNRELIABLE_TICK_RATE)
				sf::sleep(sf::seconds(UNRELIABLE_TICK_RATE - dt));
			world.update(UNRELIABLE_TICK_RATE);

			// flood snapshots to all known clients
			sf::Packet snapPkt;
			world.serialize(snapPkt);
			for(auto &p : lobbyPlayers)
			{
				if(!p.bValid)
					continue;
				if(battleSock.send(snapPkt, p.udpAddr, p.udpPort) != sf::Socket::Status::Done)
					SPDLOG_LOGGER_ERROR(logger, "Failed to send snapshot to {}:{}",
				                    p.udpAddr.toString(), p.udpPort);
			}

			numAlive = 0;
			for(auto &p : world.getPlayers())
				numAlive += p.isAlive();
			if(numAlive <= 1)
			{
				SPDLOG_LOGGER_INFO(logger, "Game ended, alive players: {}. Returning to lobby.", numAlive);
				break;
			}
		}
	}

	sf::Packet gameEndPkt;
	gameEndPkt << uint8_t(ReliablePktType::GAME_END);
	for(auto &p : world.getPlayers()) //TODO, don't want to iterate over players again here...
		if(p.isAlive())
		{
			gameEndPkt << p.m_id;
			break;
		}

	multiSock.clear();
	listener.close();
	battleSock.unbind();
	SPDLOG_LOGGER_INFO(logger, "Server shutting down.");

	return 0;

}
