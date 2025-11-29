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

static constexpr int SERVER_STARTUP_DELAY_MS = 500;

static void cleanupGameLoop(LobbyClient &lobbyClient)
{
	lobbyClient.m_lobbySock.setBlocking(true);
}

void runServer(std::shared_ptr<spdlog::logger> logger, LobbyServer *lobbyServer)
{
	SPDLOG_LOGGER_INFO(logger, "Starting dedicated server thread");

	GameServer gameServer(*lobbyServer, PORT_UDP, logger);

	while(!lobbyServer->isShutdownRequested())
	{
		lobbyServer->lobbyLoop();

		// if shutdown was requested during lobby loop
		if(lobbyServer->isShutdownRequested())
		{
			SPDLOG_LOGGER_INFO(logger, "Server shutdown requested, exiting server thread");
			break;
		}

		int connectedPlayers = 0;
		for(const auto &p : lobbyServer->m_slots)
			if(p.bValid)
				connectedPlayers++;

		SPDLOG_LOGGER_INFO(logger, "All {} players ready. Starting game...", connectedPlayers);
		lobbyServer->startGame(gameServer.m_world);
		SPDLOG_LOGGER_INFO(logger, "Game started. Switching to UDP loop.");
		PlayerState *winningPlayer = gameServer.matchLoop();
		SPDLOG_LOGGER_INFO(logger, "Game ended, winner {}. Returning to lobby",
		                   winningPlayer ? winningPlayer->m_id : 0);
	}

	SPDLOG_LOGGER_INFO(logger, "Server thread exiting");
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

		if(menu.getState() == Menu::State::LOBBY_HOST && !hostLobbyClient)
		{
			SPDLOG_LOGGER_INFO(logger, "Hosting game as player '{}'", menu.getPlayerName());

			try
			{
				lobbyServer = std::make_unique<LobbyServer>(PORT_TCP, logger);
				serverThread = std::make_unique<std::thread>(runServer, logger, lobbyServer.get());

				sf::sleep(sf::milliseconds(SERVER_STARTUP_DELAY_MS));

				// connect as client to own server
				hostLobbyClient = std::make_unique<LobbyClient>(menu.getPlayerName());
				hostLobbyClient->connect();
				SPDLOG_LOGGER_INFO(logger, "Connected to own server, waiting in lobby...");
			}
			catch(const std::exception &e)
			{
				SPDLOG_LOGGER_ERROR(logger, "Failed to start server or connect: {}", e.what());
				hostLobbyClient.reset();

				if(lobbyServer)
				{
					lobbyServer->requestShutdown();
					if(serverThread && serverThread->joinable())
					{
						serverThread->join();
					}
					serverThread.reset();
					lobbyServer.reset();
				}
				menu.reset();
			}
		}

		// reset lobby client if we left the lobby host screen
		if(menu.getState() != Menu::State::LOBBY_HOST && hostLobbyClient)
		{
			SPDLOG_LOGGER_INFO(logger, "Left lobby host screen, disconnecting...");
			hostLobbyClient.reset();
			hostReady = false;

			if(lobbyServer)
			{
				SPDLOG_LOGGER_INFO(logger, "Requesting server shutdown...");
				lobbyServer->requestShutdown();

				if(serverThread && serverThread->joinable())
				{
					SPDLOG_LOGGER_INFO(logger, "Waiting for server thread to exit...");
					serverThread->join();
					SPDLOG_LOGGER_INFO(logger, "Server thread exited");
				}
				serverThread.reset();
				lobbyServer.reset();
				SPDLOG_LOGGER_INFO(logger, "Server cleaned up successfully");
			}
		}

		// poll lobby updates while in host lobby
		if(menu.getState() == Menu::State::LOBBY_HOST && hostLobbyClient)
		{
			try
			{
				auto gameStartResult = hostLobbyClient->checkForGameStart();
				if(gameStartResult.has_value())
				{
					SPDLOG_LOGGER_INFO(logger, "Host received GAME_START packet, starting game...");
					auto playerStates = gameStartResult.value();

					menuMusic.stop();
					runGameLoop(window, *hostLobbyClient, playerStates, logger);

					hostLobbyClient.reset();
					hostReady = false;
					menu.reset();
				}
				else
				{
					hostLobbyClient->pollLobbyUpdate();
					auto players = hostLobbyClient->getLobbyPlayers();
					menu.updateLobbyDisplay(players);

					bool allReady = true;
					int playerCount = players.size();

					for(const auto &p : players)
					{
						if(!p.bReady)
							allReady = false;
					}

					menu.updateHostButton(allReady && playerCount >= 2, playerCount >= 2, hostReady);
				}
			}
			catch(const ServerShutdownException &e)
			{
				SPDLOG_LOGGER_ERROR(logger, "Unexpected server shutdown detected for host: {}", e.what());
				hostLobbyClient.reset();
				hostReady = false;
				menu.reset();
			}
		}

		if(menu.shouldStartGame() && menu.getState() == Menu::State::LOBBY_HOST && hostLobbyClient)
		{
			auto players = hostLobbyClient->getLobbyPlayers();
			bool allReady = true;
			for(const auto &p : players)
			{
				if(!p.bReady)
					allReady = false;
			}

			if(!hostReady)
			{
				SPDLOG_LOGGER_INFO(logger, "Host marked as ready");
				hostLobbyClient->sendReady();
				hostReady = true;
			}
			else if(!allReady || players.size() < 2)
			{
				SPDLOG_LOGGER_INFO(logger, "Host marked as unready");
				hostLobbyClient->sendUnready();
				hostReady = false;
			}
			else
			{
				SPDLOG_LOGGER_INFO(logger, "Host starting game...");
				hostLobbyClient->sendStartGame();
			}

			menu.clearStartGameFlag();
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
				menu.clearConnectFlag();

				menu.setState(Menu::State::LOBBY_CLIENT);
				menu.setTitle("IN LOBBY");
				menu.setupLobbyClient();

				SPDLOG_LOGGER_INFO(logger, "Connected to server, joined lobby");
			}
			catch(const std::exception &e)
			{
				SPDLOG_LOGGER_ERROR(logger, "Failed to connect to server: {}", e.what());
				joinLobbyClient.reset();
				menu.clearConnectFlag();
			}
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
				auto gameStartResult = joinLobbyClient->checkForGameStart();
				if(gameStartResult.has_value())
				{
					SPDLOG_LOGGER_INFO(logger, "Client received GAME_START packet, starting game...");
					auto playerStates = gameStartResult.value();

					menuMusic.stop();

					runGameLoop(window, *joinLobbyClient, playerStates, logger);

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
			else
			{
				SPDLOG_LOGGER_INFO(logger, "Client marked as unready");
				joinLobbyClient->sendUnready();
				clientReady = false;
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
