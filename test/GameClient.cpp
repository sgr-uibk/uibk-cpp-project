#include <iostream>
#include <SFML/Network.hpp>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <spdlog/spdlog.h>

#include "World/WorldClient.h"
#include "Player/PlayerState.h"
#include "Utilities.h"
#include "Map/MapLoader.h"

using namespace std::literals;

static constexpr unsigned short SERVER_PORT = 54000u;
static constexpr unsigned short CLIENT_PORT = 54001u;
static constexpr sf::Vector2u WINDOW_DIM{800, 600};

void updateView(sf::RenderWindow& window, sf::View& view, sf::Vector2u baseSize)
{
    float windowWidth  = static_cast<float>(window.getSize().x);
    float windowHeight = static_cast<float>(window.getSize().y);
    float windowRatio  = windowWidth / windowHeight;
    float baseRatio    = static_cast<float>(baseSize.x) / static_cast<float>(baseSize.y);

    float viewportX = 0.f;
    float viewportY = 0.f;
    float viewportWidth = 1.f;
    float viewportHeight = 1.f;

    if (windowRatio > baseRatio)
    {
        viewportWidth = baseRatio / windowRatio;
        viewportX = (1.f - viewportWidth) / 2.f;
    }
    else
    {
        viewportHeight = windowRatio / baseRatio;
        viewportY = (1.f - viewportHeight) / 2.f;
    }

    view.setViewport(sf::FloatRect({viewportX, viewportY}, {viewportWidth, viewportHeight}));
    view.setSize({static_cast<float>(baseSize.x), static_cast<float>(baseSize.y)});
    view.setCenter({static_cast<float>(baseSize.x) / 2.f, static_cast<float>(baseSize.y) / 2.f});
}

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

	MapState mapState{sf::Vector2f{static_cast<float>(WINDOW_DIM.x), static_cast<float>(WINDOW_DIM.y)}};
	MapLoader::loadMap(mapState, "../assets/maps/map1.txt", {800.f, 600.f});

	PlayerState psThePlayer(1, mapState.getPlayerSpawn(1));
	PlayerState psEnemy2 = {2, mapState.getPlayerSpawn(2)};
	PlayerState psEnemy3 = {3, mapState.getPlayerSpawn(3)};
	PlayerState psEnemy4 = {4, mapState.getPlayerSpawn(4)};

	PlayerClient pcThePlayer{psThePlayer, sf::Color::Yellow};
	PlayerClient pcEnemy2 = {psEnemy2, sf::Color::Red};
	PlayerClient pcEnemy3 = {psEnemy3, sf::Color::Cyan};
	PlayerClient pcEnemy4 = {psEnemy4, sf::Color::Magenta};
	std::array players{pcThePlayer, pcEnemy2, pcEnemy3, pcEnemy4};

	WorldClient worldClient(mapState, 1, players);

	// local predicted state
	//WorldClient worldClient(sf::Vector2f(WINDOW_DIM), 1, players);

	sf::Vector2u lastSize = window.getSize();

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

		sf::Vector2u currentSize = window.getSize();
		if(currentSize != lastSize)
		{
			lastSize = currentSize;
			updateView(window, worldClient.getWorldView(), WINDOW_DIM);
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
			WorldState snapshot(mapState);
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
