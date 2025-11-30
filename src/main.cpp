#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <thread>
#include <memory>
#include <spdlog/spdlog.h>

#include "Menu.h"
#include "Networking.h"
#include "Utilities.h"
#include "ResourceManager.h"
#include "Lobby/LobbyClient.h"
#include "Lobby/LobbyServer.h"
#include "Game/GameClient.h"
#include "Game/GameServer.h"
#include "World/WorldClient.h"

static void cleanupGameLoop(LobbyClient &lobbyClient)
{
	lobbyClient.m_lobbySock.setBlocking(true);
}

void runGameLoop(sf::RenderWindow &window, LobbyClient &lobbyClient, std::array<PlayerState, MAX_PLAYERS> &playerStates,
                 std::shared_ptr<spdlog::logger> logger)
{
	SPDLOG_LOGGER_INFO(logger, "Starting game with player ID {}", lobbyClient.m_clientId);

	lobbyClient.m_lobbySock.setBlocking(false);

	int validPlayerCount = 0;
	for(size_t i = 0; i < playerStates.size(); ++i)
	{
		if(playerStates[i].m_id != 0)
		{
			validPlayerCount++;
			SPDLOG_LOGGER_INFO(logger, "Player {}: pos=({:.1f},{:.1f}), rot={:.1f}°, hp={}", playerStates[i].m_id,
			                   playerStates[i].getPosition().x, playerStates[i].getPosition().y,
			                   playerStates[i].getRotation().asDegrees(), playerStates[i].getHealth());
		}
	}
	SPDLOG_LOGGER_INFO(logger, "Starting game loop with {} valid players", validPlayerCount);
	WorldClient worldClient(window, lobbyClient.m_clientId, playerStates);
	GameClient gameClient(worldClient, lobbyClient, logger);

	while(window.isOpen())
	{
		gameClient.handleUserInputs(window);

		if(worldClient.m_pauseMenu.isDisconnectRequested())
		{
			SPDLOG_LOGGER_INFO(logger, "Player disconnected via pause menu, returning to lobby...");
			break;
		}

		gameClient.syncFromServer();

		if(gameClient.processReliablePackets(lobbyClient.m_lobbySock))
		{
			SPDLOG_LOGGER_INFO(logger, "Game ended, returning to lobby...");
			break;
		}
	}

	cleanupGameLoop(lobbyClient);
	SPDLOG_LOGGER_INFO(logger, "Game window closed, returning to lobby...");
}

int main()
{
	auto logger = createConsoleAndFileLogger("TankGame");
	SPDLOG_LOGGER_INFO(logger, "Tank Game starting...");

	sf::RenderWindow window(sf::VideoMode(WINDOW_DIM), "Tank Game", sf::Style::Titlebar | sf::Style::Close);
	window.setFramerateLimit(60);

	Menu menu(WINDOW_DIM);
	sf::Music &menuMusic = initMusic("audio/menu_loop.ogg");
	std::unique_ptr<std::thread> serverThread;
	std::unique_ptr<LobbyServer> lobbyServer;
	std::unique_ptr<LobbyClient> hostLobbyClient;
	std::unique_ptr<LobbyClient> joinLobbyClient;
	bool hostReady = false;
	bool clientReady = false;

	while(window.isOpen())
	{
		while(const std::optional event = window.pollEvent())
		{
			if(event->is<sf::Event::Closed>())
				window.close();
			else if(const auto *mouseButtonPressed = event->getIf<sf::Event::MouseButtonPressed>())
			{
				if(mouseButtonPressed->button == sf::Mouse::Button::Left)
					menu.handleClick(window.mapPixelToCoords(mouseButtonPressed->position, window.getView()));
			}
			else if(const auto *mouseMoved = event->getIf<sf::Event::MouseMoved>())
				menu.handleMouseMove(window.mapPixelToCoords(mouseMoved->position, window.getView()));
			else if(const auto *textEntered = event->getIf<sf::Event::TextEntered>())
			{
				if(textEntered->unicode < 128)
					menu.handleTextInput(static_cast<char>(textEntered->unicode));
			}
		}

		if(menu.isMenuMusicEnabled())
		{
			if(menuMusic.getStatus() != sf::Music::Status::Playing)
				menuMusic.play();
		}
		else
		{
			if(menuMusic.getStatus() == sf::Music::Status::Playing)
				menuMusic.stop();
		}

		if(menu.shouldExit())
		{
			window.close();
			break;
		}

		if(menu.shouldConnect() && menu.getState() == Menu::State::JOIN_LOBBY && !joinLobbyClient)
		{
			SPDLOG_LOGGER_INFO(logger, "Connecting to game at {}:{} as player '{}'", menu.getServerIp(),
			                   menu.getServerPort(), menu.getPlayerName());

			try
			{
				auto ipAddress = sf::IpAddress::resolve(menu.getServerIp());
				if(!ipAddress.has_value())
				{
					throw std::runtime_error("Failed to resolve IP address: " + menu.getServerIp());
				}

				joinLobbyClient = std::make_unique<LobbyClient>(menu.getPlayerName(),
				                                                Endpoint{ipAddress.value(), menu.getServerPort()});
				joinLobbyClient->connect();

				menu.setState(Menu::State::LOBBY_CLIENT);
				menu.setTitle("IN LOBBY");
				menu.setupLobbyClient();

				SPDLOG_LOGGER_INFO(logger, "Connected to server, joined lobby");
			}
			catch(const std::exception &e)
			{
				SPDLOG_LOGGER_ERROR(logger, "Failed to connect to server: {}", e.what());
				joinLobbyClient.reset();
			}
			menu.clearConnectFlag();
		}

		// reset join lobby client if we left the lobby client screen
		if(menu.getState() != Menu::State::LOBBY_CLIENT && joinLobbyClient)
		{
			SPDLOG_LOGGER_INFO(logger, "Left lobby client screen, disconnecting...");
			joinLobbyClient.reset();
			clientReady = false;
		}

		if(menu.getState() == Menu::State::LOBBY_CLIENT && joinLobbyClient)
		{
			try
			{
				auto gameStartResult = joinLobbyClient->waitForGameStart(sf::milliseconds(100));
				if(gameStartResult.has_value())
				{
					SPDLOG_LOGGER_INFO(logger, "Client received GAME_START packet, starting game...");

					menuMusic.stop();

					runGameLoop(window, *joinLobbyClient, *gameStartResult, logger);

					joinLobbyClient.reset();
					clientReady = false;
					menu.reset();
				}
				else
				{
					joinLobbyClient->pollLobbyUpdate();
					menu.updateLobbyDisplay(joinLobbyClient->getLobbyPlayers());
					menu.updateClientButton(clientReady);
				}
			}
			catch(const ServerShutdownException &e)
			{
				SPDLOG_LOGGER_WARN(logger, "Server has shut down: {}", e.what());
				joinLobbyClient.reset();
				clientReady = false;
				menu.reset();
			}
		}

		if(menu.shouldStartGame() && menu.getState() == Menu::State::LOBBY_CLIENT && joinLobbyClient)
		{
			if(!clientReady)
			{
				SPDLOG_LOGGER_INFO(logger, "Client marked as ready");
				joinLobbyClient->sendReady();
				clientReady = true;
			}
			menu.clearStartGameFlag();
		}

		window.clear(sf::Color(20, 20, 20));
		menu.draw(window);
		window.display();
	}

	SPDLOG_LOGGER_INFO(logger, "Tank Game shutting down");
	return 0;
}
