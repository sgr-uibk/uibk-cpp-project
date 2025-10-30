#include <iostream>
#include <SFML/Network.hpp>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <spdlog/spdlog.h>

#include "World/WorldClient.h"
#include "Player/PlayerState.h"
#include "Utilities.h"

using namespace std::literals;

static constexpr unsigned short SERVER_PORT = 54000u;
static constexpr unsigned short CLIENT_PORT = 54001u;
static constexpr sf::Vector2u WINDOW_DIM{800, 600};

int main()
{
	std::shared_ptr<spdlog::logger> const logger = createConsoleLogger("Unreliable Client");

	sf::RenderWindow window(sf::VideoMode(WINDOW_DIM), "Unreliable Client", sf::Style::Resize);
	window.setFramerateLimit(60);

	// UDP socket
	sf::UdpSocket socket;
	if(socket.bind(CLIENT_PORT) != sf::Socket::Status::Done)
	{
		std::cerr << "Client: bind failed\n";
		return 1;
	}
	socket.setBlocking(false);

	sf::IpAddress serverAddr = sf::IpAddress::LocalHost;

	PlayerState psThePlayer(1, {100.f, 100.f});
	PlayerState psEnemy2 = {2, {300, 400}};
	PlayerState psEnemy3 = {3, {350, 250}};
	PlayerState psEnemy4 = {4, {500, 100}};

	PlayerClient pcThePlayer{psThePlayer, sf::Color::Yellow};
	PlayerClient pcEnemy2 = {psEnemy2, sf::Color::Red};
	PlayerClient pcEnemy3 = {psEnemy3, sf::Color::Cyan};
	PlayerClient pcEnemy4 = {psEnemy4, sf::Color::Magenta};
	std::array players{pcThePlayer, pcEnemy2, pcEnemy3, pcEnemy4};

	// local predicted state
	WorldClient worldClient(sf::Vector2f(WINDOW_DIM), 1, players);

	sf::Clock frameClock;
	while(window.isOpen())
	{
		while(const std::optional event = window.pollEvent())
		{
			if(event->is<sf::Event::Closed>())
			{
				window.close();
			}
			else if(const auto *keyPressed = event->getIf<sf::Event::KeyPressed>())
			{
				if(keyPressed->scancode == sf::Keyboard::Scancode::Escape)
					window.close();
			}
		}

		float dt = frameClock.restart().asSeconds();
		sf::Packet pkt = worldClient.update(dt);

		if(pkt.getDataSize() > 0)
		{
			if(sf::Socket::Status::Done != socket.send(pkt, serverAddr, SERVER_PORT))
				SPDLOG_LOGGER_ERROR(logger, "Socket send failed");
		}
		// receive snapshot from server (non-blocking)
		sf::Packet pktSnap;
		std::optional<sf::IpAddress> srcAddr;
		unsigned short srcPort;
		if(socket.receive(pktSnap, srcAddr, srcPort) == sf::Socket::Status::Done)
		{
			WorldState snapshot{sf::Vector2f(WINDOW_DIM)};
			snapshot.deserialize(pktSnap);
			worldClient.applyServerSnapshot(snapshot);
			auto const ops = worldClient.getOwnPlayerState();
			SPDLOG_LOGGER_INFO(logger, "applied ss, pos=({},{}), rotDeg={}, h={}", ops.m_pos.x, ops.m_pos.y,
			                   ops.m_rot.asDegrees(), ops.m_health);
		}

		window.clear();
		worldClient.draw(window);
		window.display();
	}

	return 0;
}
