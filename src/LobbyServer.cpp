#include "Networking.h"
#include <SFML/Network.hpp>
#include <iostream>
#include <string>
#include <spdlog/spdlog.h>
#include "Utilities.h"

struct LobbyPlayer
{
	uint32_t id;
	bool bValid{false};
	bool bReady{false};
	std::string name;
	sf::TcpSocket socket;
};

int main(int argc, char**argv)
{
	auto logger = createConsoleAndFileLogger("Server L");
	sf::TcpListener listener;
	uint16_t const port = (argc == 2) ? atoi(argv[1]) : PORT_TCP;
	if(listener.listen(port) != sf::Socket::Status::Done)
	{
		std::cerr << "Failed to listen TCP";
		return 1;
	}
	sf::SocketSelector multiSock;
	multiSock.add(listener);

	std::array<LobbyPlayer, MAX_PLAYERS> players;
	uint32_t nextClientId = 1;

	SPDLOG_LOGGER_INFO(logger, "Lobby server listening on TCP port {}", port);

	while(true)
	{
		if(multiSock.wait(sf::milliseconds(250)))
		{
			// Got a message. Is it a new client ?
			if(multiSock.isReady(listener))
			{
				LobbyPlayer p{};
				if(listener.accept(p.socket) == sf::Socket::Status::Done)
				{
					if(nextClientId > MAX_PLAYERS)
					{
						SPDLOG_LOGGER_INFO(logger, "Lobby has reached the maximum of {} members. Rejecting.",
						             MAX_PLAYERS);
						listener.close();
						assert(!multiSock.isReady(listener));
					}
					p.socket.setBlocking(false);
					p.id = nextClientId++;
					if(sf::Packet pktJoinRequest; p.socket.receive(pktJoinRequest) == sf::Socket::Status::Done)
					{
						uint32_t protoVer;
						pktJoinRequest >> protoVer;
						if(protoVer != PROTOCOL_VERSION)
						{
							SPDLOG_LOGGER_ERROR(logger, "Protocol version mismatch: Expected {}, got {}.",
							              PROTOCOL_VERSION, protoVer);
							goto ABORT_CONNECTION;
						}
						pktJoinRequest >> p.name;
					}
					else
					{
					ABORT_CONNECTION:
						p.socket.disconnect();
						continue;
					}

					// now acknowledge the client's join request
					sf::Packet pktJoinAck;
					pktJoinAck << (uint8_t)ReliablePktType::JOIN_ACK;
					pktJoinAck << p.id;

					if(sf::Socket::Status status = p.socket.send(pktJoinAck); status != sf::Socket::Status::Done)
						SPDLOG_LOGGER_ERROR(logger, "Send JOIN_ACK returned Status {}", (int)status);

					SPDLOG_LOGGER_INFO(logger, "Player {} (id {}) joined lobby", p.name, p.id);

					// remember the player and moitor their socket
					p.bValid = true;
					multiSock.add(p.socket);
					players[p.id - 1] = std::move(p);
				}
			}

			// Do the known clients have something to say ?
			size_t cReady = 0;
			for(auto &p : players)
			{
				auto &sock = p.socket;
				sf::Packet pktLobby;

				if(multiSock.isReady(sock))
					switch(sock.receive(pktLobby))
					{
					case sf::Socket::Status::Disconnected:
						SPDLOG_LOGGER_INFO(logger, "Player {} (id {}) left lobby", p.name, p.id);
						sock.disconnect();
						multiSock.remove(sock);
						p.bValid = false;
						break;

					case sf::Socket::Status::Done: {
						uint8_t type;
						pktLobby >> type;

						if(type == (uint8_t)ReliablePktType::LOBBY_READY)
						{
							uint32_t clientId;
							pktLobby >> clientId;
							assert(p.id == clientId);
							SPDLOG_LOGGER_INFO(logger, "Player {} (id {}) is ready", p.name, p.id);
							p.bReady = true;
						}
						break;
					}
				default:
						SPDLOG_LOGGER_WARN(logger, "Got an unhandled Pkt type in Lobby");
					}
				cReady += p.bReady;
			}

			if(cReady == MAX_PLAYERS)
			{
				SPDLOG_LOGGER_INFO(logger, "All {} players are ready!", cReady);
				break;
			}
		}
		sf::sleep(sf::milliseconds(100));
	}

	sf::Packet pktGameStart;
	srand(0); // deteministic

	for(auto &p : players)
	{
		pktGameStart.clear();

		sf::Vector2f spawnPoint = {float(rand()) / float(RAND_MAX / 600),
		                           float(rand()) / float(RAND_MAX / 800)};
		sf::Angle direction = sf::degrees(float(rand()) / float(RAND_MAX / 360));

		pktGameStart << (uint8_t)ReliablePktType::GAME_START;
		pktGameStart << spawnPoint;
		pktGameStart << direction.asRadians();

		if(sf::Socket::Status status = p.socket.send(pktGameStart); status != sf::Socket::Status::Done)
			SPDLOG_LOGGER_ERROR(logger, "Send GAME_START returned Status {}", (int)status);
	}

	SPDLOG_LOGGER_INFO(logger, "Game started... Let them battle...");
	sf::sleep(sf::seconds(5)); // simulate battle

	SPDLOG_LOGGER_INFO(logger, "Server shutting down.");

	return 0;
}
