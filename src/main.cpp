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

static void cleanupGameLoop(sf::Music *battleMusicPtr, LobbyClient &lobbyClient)
{
	if(battleMusicPtr)
		battleMusicPtr->stop();
}

static void handleReturnToLobby(LobbyClient &lobbyClient, const std::shared_ptr<spdlog::logger> &logger)
{
	SPDLOG_LOGGER_INFO(logger, "Returned to lobby, keeping connection alive");

	bool gotUpdate = false;
	for(int i = 0; i < 10; ++i)
	{
		if(lobbyClient.pollLobbyUpdate())
			gotUpdate = true;
		sf::sleep(sf::milliseconds(50));
	}

	if(!gotUpdate)
	{
		SPDLOG_LOGGER_WARN(logger, "Did not receive lobby update after returning from game");
	}
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
		lobbyServer->setGameInProgress(true);
		SPDLOG_LOGGER_INFO(logger, "Game started. Switching to UDP loop.");
		PlayerState *winningPlayer = gameServer.matchLoop();
		lobbyServer->setGameInProgress(false);
		SPDLOG_LOGGER_INFO(logger, "Game ended, winner {}. Returning to lobby",
		                   winningPlayer ? winningPlayer->m_id : 0);

		sf::Packet gameEndPkt = createPkt(ReliablePktType::GAME_END);
		gameEndPkt << static_cast<uint32_t>(winningPlayer ? winningPlayer->m_id : 0);

		const auto &players = gameServer.m_world.getPlayers();
		uint32_t numPlayers = 0;
		for(const auto &player : players)
		{
			if(player.m_id != 0)
				numPlayers++;
		}

		gameEndPkt << numPlayers;
		for(const auto &player : players)
		{
			if(player.m_id != 0)
			{
				gameEndPkt << player.m_id;
				gameEndPkt << player.m_name;
				gameEndPkt << static_cast<int32_t>(player.getKills());
				gameEndPkt << static_cast<int32_t>(player.getDeaths());
				SPDLOG_LOGGER_DEBUG(logger, "Player {}: {} - K:{} D:{}", player.m_id, player.m_name, player.getKills(),
				                    player.getDeaths());
			}
		}

		for(auto &p : lobbyServer->m_slots)
		{
			if(p.bValid)
			{
				if(checkedSend(p.tcpSocket, gameEndPkt) != sf::Socket::Status::Done)
				{
					SPDLOG_LOGGER_ERROR(logger, "Failed to send GAME_END to {} (id {})", p.name, p.id);
				}
				else
				{
					SPDLOG_LOGGER_INFO(logger, "Sent GAME_END to {} (id {})", p.name, p.id);
				}
			}
		}

		lobbyServer->updatePlayerStats(players);

		lobbyServer->resetLobbyState();
		SPDLOG_LOGGER_INFO(logger, "Returning to lobby loop, waiting for players to ready up...");
	}

	SPDLOG_LOGGER_INFO(logger, "Server thread exiting");
}

GameEndResult runGameLoop(sf::RenderWindow &window, LobbyClient &lobbyClient,
                          std::array<PlayerState, MAX_PLAYERS> &playerStates, std::shared_ptr<spdlog::logger> logger,
                          bool gameMusicEnabled)
{
	SPDLOG_LOGGER_INFO(logger, "Starting game with player ID {}", lobbyClient.m_clientId);

	sf::Music *battleMusicPtr = nullptr;
	try
	{
		battleMusicPtr = &MusicManager::inst().load("audio/battle_loop.ogg");
		battleMusicPtr->setLooping(true);
		if(gameMusicEnabled)
		{
			battleMusicPtr->play();
			SPDLOG_LOGGER_INFO(logger, "Battle music started");
		}
	}
	catch(const std::exception &e)
	{
		SPDLOG_LOGGER_WARN(logger, "Failed to load battle music: {}. Continuing without music.", e.what());
		battleMusicPtr = nullptr;
	}

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

	WorldClient worldClient(window, lobbyClient.m_clientId, playerStates, battleMusicPtr);

	GameClient gameClient(worldClient, lobbyClient, logger);

	int frameCount = 0;
	while(window.isOpen())
	{
		gameClient.handleUserInputs(window);

		if(worldClient.shouldDisconnect())
		{
			cleanupGameLoop(battleMusicPtr, lobbyClient);
			SPDLOG_LOGGER_INFO(logger, "Player disconnected via pause menu, returning to main menu...");
			return GameEndResult::RETURN_TO_MAIN_MENU;
		}

		if(worldClient.shouldReturnToLobby())
		{
			cleanupGameLoop(battleMusicPtr, lobbyClient);
			SPDLOG_LOGGER_INFO(logger, "Player chose to return to lobby from scoreboard...");
			return GameEndResult::RETURN_TO_LOBBY;
		}

		gameClient.syncFromServer();

		gameClient.processReliablePackets(lobbyClient.m_lobbySock);

		// log every 60 frames (~once per second at 60 fps)
		if(++frameCount % 60 == 0)
		{
			SPDLOG_LOGGER_DEBUG(logger, "Game loop frame {}", frameCount);
		}
	}

	cleanupGameLoop(battleMusicPtr, lobbyClient);
	SPDLOG_LOGGER_INFO(logger, "Game window closed, returning to main menu...");
	return GameEndResult::RETURN_TO_MAIN_MENU;
}

int main()
{
	auto logger = createConsoleAndFileLogger("TankGame");
	SPDLOG_LOGGER_INFO(logger, "Tank Game starting...");

	sf::RenderWindow window(sf::VideoMode(WINDOW_DIM), "Tank Game",
	                        sf::Style::Titlebar | sf::Style::Close | sf::Style::Resize);
	window.setFramerateLimit(60);

	Menu menu(WINDOW_DIM);

	sf::Music *menuMusicPtr = nullptr;
	try
	{
		menuMusicPtr = &MusicManager::inst().load("audio/menu_loop.ogg");
		menuMusicPtr->setLooping(true);
		if(menu.isMenuMusicEnabled())
		{
			menuMusicPtr->play();
		}
		SPDLOG_LOGGER_INFO(logger, "Menu music loaded successfully");
	}
	catch(const std::exception &e)
	{
		SPDLOG_LOGGER_WARN(logger, "Failed to load menu music: {}. Continuing without music.", e.what());
		menuMusicPtr = nullptr;
	}

	std::unique_ptr<std::thread> serverThread;
	std::unique_ptr<LobbyServer> lobbyServer;

	std::unique_ptr<LobbyClient> hostLobbyClient;
	bool hostReady = false;

	std::unique_ptr<LobbyClient> joinLobbyClient;
	bool clientReady = false;

	while(window.isOpen())
	{
		while(const std::optional event = window.pollEvent())
		{
			if(event->is<sf::Event::Closed>())
			{
				window.close();
			}
			else if(const auto *mouseButtonPressed = event->getIf<sf::Event::MouseButtonPressed>())
			{
				if(mouseButtonPressed->button == sf::Mouse::Button::Left)
				{
					menu.handleClick(window.mapPixelToCoords(mouseButtonPressed->position, window.getView()));
				}
			}
			else if(const auto *mouseMoved = event->getIf<sf::Event::MouseMoved>())
			{
				menu.handleMouseMove(window.mapPixelToCoords(mouseMoved->position, window.getView()));
			}
			else if(const auto *textEntered = event->getIf<sf::Event::TextEntered>())
			{
				if(textEntered->unicode < 128)
				{
					menu.handleTextInput(static_cast<char>(textEntered->unicode));
				}
			}
			else if(const auto *resized = event->getIf<sf::Event::Resized>())
			{
				// Uniform scaling: scale by the same factor in both directions
				float scaleX = static_cast<float>(resized->size.x) / static_cast<float>(WINDOW_DIM.x);
				float scaleY = static_cast<float>(resized->size.y) / static_cast<float>(WINDOW_DIM.y);

				float scale = std::min(scaleX, scaleY);

				float viewportWidth = (WINDOW_DIM.x * scale) / resized->size.x;
				float viewportHeight = (WINDOW_DIM.y * scale) / resized->size.y;

				float viewportX = (1.f - viewportWidth) / 2.f;
				float viewportY = (1.f - viewportHeight) / 2.f;

				sf::View view(sf::FloatRect({0.f, 0.f}, sf::Vector2f(WINDOW_DIM)));
				view.setViewport(sf::FloatRect({viewportX, viewportY}, {viewportWidth, viewportHeight}));
				window.setView(view);
				menu.handleResize(resized->size);
			}
		}

		if(menuMusicPtr)
		{
			if(menu.isMenuMusicEnabled())
			{
				if(menuMusicPtr->getStatus() != sf::Music::Status::Playing)
					menuMusicPtr->play();
			}
			else
			{
				if(menuMusicPtr->getStatus() == sf::Music::Status::Playing)
					menuMusicPtr->stop();
			}
		}

		if(menu.shouldExit())
		{
			window.close();
			break;
		}

		if(menu.getState() == Menu::State::LOBBY_HOST && !hostLobbyClient)
		{
			if(serverThread && serverThread->joinable())
			{
				SPDLOG_LOGGER_INFO(logger, "Waiting for previous server thread to exit before hosting new game...");
				if(lobbyServer)
				{
					lobbyServer->requestShutdown();
				}
				serverThread->join();
				serverThread.reset();
				lobbyServer.reset();
				SPDLOG_LOGGER_INFO(logger, "Previous server cleaned up");
			}

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
				menu.showError("Failed to host game:\n" + std::string(e.what()));
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

					if(menuMusicPtr)
						menuMusicPtr->stop();

					GameEndResult result =
						runGameLoop(window, *hostLobbyClient, playerStates, logger, menu.isGameMusicEnabled());

					if(result == GameEndResult::RETURN_TO_LOBBY)
					{
						handleReturnToLobby(*hostLobbyClient, logger);
						hostReady = false;

						auto players = hostLobbyClient->getLobbyPlayers();
						menu.updateLobbyDisplay(players);
						menu.updateHostButton(false, players.size() >= 2, hostReady);

						if(menuMusicPtr && menu.isMenuMusicEnabled())
							menuMusicPtr->play();
					}
					else // RETURN_TO_MAIN_MENU
					{
						SPDLOG_LOGGER_INFO(logger, "Returned to main menu, disconnecting from lobby");
						hostLobbyClient.reset();
						hostReady = false;
						menu.reset();
						if(menuMusicPtr && menu.isMenuMusicEnabled())
							menuMusicPtr->play();
					}
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

				if(lobbyServer)
				{
					SPDLOG_LOGGER_INFO(logger, "Cleaning up server after shutdown exception...");
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

				menu.showError("Server has shut down unexpectedly.\nReturning to main menu.");
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
				menu.showError("Failed to connect to server:\n" + std::string(e.what()));
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

					if(menuMusicPtr)
						menuMusicPtr->stop();

					GameEndResult result =
						runGameLoop(window, *joinLobbyClient, playerStates, logger, menu.isGameMusicEnabled());

					if(result == GameEndResult::RETURN_TO_LOBBY)
					{
						handleReturnToLobby(*joinLobbyClient, logger);
						clientReady = false;

						menu.updateLobbyDisplay(joinLobbyClient->getLobbyPlayers());
						menu.updateClientButton(clientReady);

						if(menuMusicPtr && menu.isMenuMusicEnabled())
							menuMusicPtr->play();
					}
					else // RETURN_TO_MAIN_MENU
					{
						SPDLOG_LOGGER_INFO(logger, "Returned to main menu, disconnecting from lobby");
						joinLobbyClient.reset();
						clientReady = false;
						menu.reset();
						if(menuMusicPtr && menu.isMenuMusicEnabled())
							menuMusicPtr->play();
					}
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
				menu.showError("Host has disconnected.\nThe server has shut down.");
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

	// cleanup server and clients before exiting
	SPDLOG_LOGGER_INFO(logger, "Tank Game shutting down, cleaning up resources...");

	if(hostLobbyClient)
	{
		SPDLOG_LOGGER_INFO(logger, "Disconnecting host lobby client...");
		hostLobbyClient.reset();
	}

	if(joinLobbyClient)
	{
		SPDLOG_LOGGER_INFO(logger, "Disconnecting join lobby client...");
		joinLobbyClient.reset();
	}

	if(lobbyServer)
	{
		SPDLOG_LOGGER_INFO(logger, "Shutting down server...");
		lobbyServer->requestShutdown();

		if(serverThread && serverThread->joinable())
		{
			SPDLOG_LOGGER_INFO(logger, "Waiting for server thread to exit...");
			serverThread->join();
			SPDLOG_LOGGER_INFO(logger, "Server thread exited successfully");
		}

		serverThread.reset();
		lobbyServer.reset();
		SPDLOG_LOGGER_INFO(logger, "Server cleaned up successfully");
	}

	SPDLOG_LOGGER_INFO(logger, "All resources cleaned up, exiting");
	return 0;
}
