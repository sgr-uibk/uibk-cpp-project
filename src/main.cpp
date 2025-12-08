#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <thread>
#include <memory>
#include <atomic>
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

// ---------------------------------------------------------
// HELPER: Thread Cleanup
// ---------------------------------------------------------
void stopAndJoinServer(std::unique_ptr<LobbyServer> &server, std::unique_ptr<std::thread> &thread)
{
	if(server)
	{
		server->stop();
	}

	if(thread && thread->joinable())
	{
		thread->join();
	}

	server.reset();
	thread.reset();
}

// ---------------------------------------------------------
// SERVER THREAD
// ---------------------------------------------------------
void runServerThread(LobbyServer *lobbyServer, std::shared_ptr<spdlog::logger> logger)
{
	SPDLOG_LOGGER_INFO(logger, "Server thread started. Waiting for players...");

	while(lobbyServer->isRunning())
	{
		SPDLOG_LOGGER_INFO(logger, "Entering lobby phase...");
		lobbyServer->lobbyLoop();

		if(!lobbyServer->isRunning())
		{
			SPDLOG_LOGGER_INFO(logger, "Server stopped during lobby phase.");
			break;
		}

		SPDLOG_LOGGER_INFO(logger, "Lobby finished. Initializing Game Server...");

		try
		{
			std::array<PlayerState, MAX_PLAYERS> emptyPlayers{};
			WorldState initialWorld = WorldState::fromTiledMap("map/arena.json", emptyPlayers);
			if(initialWorld.getMap().getSpawns().empty())
			{
				SPDLOG_LOGGER_ERROR(logger, "Server failed to load map! Using empty world.");
			}
			lobbyServer->startGame(initialWorld);
			GameServer gameServer(*lobbyServer, PORT_UDP, initialWorld);

			SPDLOG_LOGGER_INFO(logger, "Game Started! Entering Match Loop.");

			PlayerState *winner = gameServer.matchLoop();

			lobbyServer->updatePlayerStats(gameServer.m_world.getPlayers());

			if(winner)
			{
				SPDLOG_LOGGER_INFO(logger, "Match ended. Winner ID: {}", winner->m_id);
				lobbyServer->endGame(winner->m_id);
			}
			else
			{
				SPDLOG_LOGGER_INFO(logger, "Match ended in a draw.");
				lobbyServer->endGame(0);
			}

			lobbyServer->resetLobbyState();
			SPDLOG_LOGGER_INFO(logger, "Lobby reset. Ready for next game.");
		}
		catch(std::exception &e)
		{
			SPDLOG_LOGGER_ERROR(logger, "Server crash: {}", e.what());
			break;
		}
	}

	SPDLOG_LOGGER_INFO(logger, "Server thread finished.");
}

// ---------------------------------------------------------
// CLIENT GAME LOOP
// Returns true if player wants to return to lobby, false if they want to disconnect
// ---------------------------------------------------------
bool runClientGameLoop(sf::RenderWindow &window, LobbyClient &lobbyClient,
                       std::array<PlayerState, MAX_PLAYERS> &playerStates)
{
	SPDLOG_LOGGER_INFO(spdlog::get("Client"), "Starting game with player ID {}", lobbyClient.m_clientId);

	lobbyClient.m_lobbySock.setBlocking(false);

	WorldClient worldClient(window, lobbyClient.m_clientId, playerStates);
	GameClient gameClient(worldClient, lobbyClient);

	bool gameEnded = false;
	bool returnToLobby = false;

	while(window.isOpen())
	{
		gameClient.update(window);

		if(worldClient.m_pauseMenu.isDisconnectRequested())
		{
			SPDLOG_LOGGER_INFO(spdlog::get("Client"), "Player disconnected via pause menu");
			returnToLobby = false;
			break;
		}

		if(worldClient.isReturnToLobbyRequested())
		{
			SPDLOG_LOGGER_INFO(spdlog::get("Client"), "Returning to lobby via Scoreboard");
			returnToLobby = true;
			break;
		}

		gameClient.processUnreliablePackets();

		if(gameClient.processReliablePackets(lobbyClient.m_lobbySock))
		{
			if(!gameEnded)
			{
				SPDLOG_LOGGER_INFO(spdlog::get("Client"), "Game Over received. Waiting for user to return to lobby...");
				gameEnded = true;
			}
		}
	}
	worldClient.m_pauseMenu.stopBattleMusic();

	lobbyClient.m_lobbySock.setBlocking(true);
	return returnToLobby;
}

// ---------------------------------------------------------
// MENU & LOBBY HELPERS
// ---------------------------------------------------------
void handleMenuEvents(sf::RenderWindow &window, Menu &menu)
{
	while(std::optional const event = window.pollEvent())
	{
		if(event->is<sf::Event::Closed>())
			window.close();
		else if(auto const *mouseBtn = event->getIf<sf::Event::MouseButtonPressed>())
		{
			if(mouseBtn->button == sf::Mouse::Button::Left)
				menu.handleClick(window.mapPixelToCoords(mouseBtn->position, window.getView()));
		}
		else if(auto const *mouseMv = event->getIf<sf::Event::MouseMoved>())
			menu.handleMouseMove(window.mapPixelToCoords(mouseMv->position, window.getView()));
		else if(auto const *textEvt = event->getIf<sf::Event::TextEntered>())
		{
			if(textEvt->unicode < 128)
				menu.handleTextInput(static_cast<char>(textEvt->unicode));
		}
	}
}

void updateMenuMusic(Menu &menu, sf::Music &menuMusic)
{
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
}

void handleHostGame(Menu &menu, std::unique_ptr<LobbyServer> &lobbyServer, std::unique_ptr<std::thread> &serverThread,
                    std::unique_ptr<LobbyClient> &joinLobbyClient, std::shared_ptr<spdlog::logger> clientLogger,
                    std::shared_ptr<spdlog::logger> serverLogger)
{
	try
	{
		stopAndJoinServer(lobbyServer, serverThread);
		SPDLOG_LOGGER_INFO(clientLogger, "Initializing Host on Port {}...", PORT_TCP);

		lobbyServer = std::make_unique<LobbyServer>(PORT_TCP);
		serverThread = std::make_unique<std::thread>(runServerThread, lobbyServer.get(), serverLogger);

		sf::sleep(sf::milliseconds(200));

		joinLobbyClient =
			std::make_unique<LobbyClient>(menu.getPlayerName(), Endpoint{sf::IpAddress::LocalHost, PORT_TCP});
		joinLobbyClient->connect();

		menu.setupLobbyHost();
		SPDLOG_LOGGER_INFO(clientLogger, "Host started successfully.");
	}
	catch(std::exception const &e)
	{
		SPDLOG_LOGGER_ERROR(clientLogger, "Host Failed: {}", e.what());
		menu.showError("Failed to Host:\n" + std::string(e.what()));
		stopAndJoinServer(lobbyServer, serverThread);
		joinLobbyClient.reset();
	}
}

void handleJoinGame(Menu &menu, std::unique_ptr<LobbyClient> &joinLobbyClient,
                    std::shared_ptr<spdlog::logger> clientLogger)
{
	try
	{
		auto ip = sf::IpAddress::resolve(menu.getServerIp());
		if(!ip)
			throw std::runtime_error("Invalid IP Address");

		joinLobbyClient = std::make_unique<LobbyClient>(
			menu.getPlayerName(), Endpoint{ip.value(), static_cast<uint16_t>(menu.getServerPort())});
		joinLobbyClient->connect();

		menu.setState(Menu::State::LOBBY_CLIENT);
		menu.setTitle("IN LOBBY");
		menu.setupLobbyClient();
	}
	catch(std::exception const &e)
	{
		SPDLOG_LOGGER_ERROR(clientLogger, "Join Failed: {}", e.what());
		menu.showError("Connection Failed:\n" + std::string(e.what()));
		joinLobbyClient.reset();
	}
}

void handleLobbyCleanup(Menu &menu, std::unique_ptr<LobbyClient> &joinLobbyClient,
                        std::unique_ptr<LobbyServer> &lobbyServer, std::unique_ptr<std::thread> &serverThread,
                        bool &clientReady, std::shared_ptr<spdlog::logger> clientLogger)
{
	if(menu.getState() != Menu::State::LOBBY_CLIENT && menu.getState() != Menu::State::LOBBY_HOST &&
	   (joinLobbyClient || lobbyServer))
	{
		SPDLOG_LOGGER_INFO(clientLogger, "Leaving lobby, cleaning up resources...");
		joinLobbyClient.reset();
		clientReady = false;
		stopAndJoinServer(lobbyServer, serverThread);
		SPDLOG_LOGGER_INFO(clientLogger, "Cleanup complete.");
	}
}

void handleLobbyClientUpdates(sf::RenderWindow &window, Menu &menu, sf::Music &menuMusic,
                              std::unique_ptr<LobbyClient> &joinLobbyClient, std::unique_ptr<LobbyServer> &lobbyServer,
                              std::unique_ptr<std::thread> &serverThread, bool &clientReady,
                              std::shared_ptr<spdlog::logger> clientLogger)
{
	if(!joinLobbyClient)
		return;

	try
	{
		LobbyEvent event = joinLobbyClient->pollLobbyUpdate();

		if(event == LobbyEvent::LOBBY_UPDATED)
		{
			menu.updateLobbyDisplay(joinLobbyClient->getLobbyPlayers());
			if(clientReady)
				menu.disableReadyButton();
		}
		else if(event == LobbyEvent::SERVER_DISCONNECTED)
		{
			SPDLOG_LOGGER_ERROR(clientLogger, "Server disconnected (host left)");
			menu.showError("Server disconnected\n(Host left the lobby)");
			menu.reset();
			joinLobbyClient.reset();
			stopAndJoinServer(lobbyServer, serverThread);
			clientReady = false;
		}
		else if(event == LobbyEvent::GAME_STARTED)
		{
			SPDLOG_LOGGER_INFO(clientLogger, "Game Starting...");
			menuMusic.stop();

			auto startData = joinLobbyClient->getGameStartData();
			bool shouldReturnToLobby = runClientGameLoop(window, *joinLobbyClient, startData);

			if(shouldReturnToLobby)
			{
				SPDLOG_LOGGER_INFO(clientLogger, "Returned to lobby, waiting for lobby update...");
				auto players = joinLobbyClient->getLobbyPlayers();
				for(auto &p : players)
				{
					p.bReady = false;
				}
				menu.updateLobbyDisplay(players);
				menu.enableReadyButton();
				menuMusic.play();
				clientReady = false;
			}
			else
			{
				SPDLOG_LOGGER_INFO(clientLogger, "Player disconnected, cleaning up...");
				menu.reset();
				joinLobbyClient.reset();
				stopAndJoinServer(lobbyServer, serverThread);
				clientReady = false;
			}
		}
	}
	catch(std::exception const &e)
	{
		SPDLOG_LOGGER_WARN(clientLogger, "Disconnected: {}", e.what());
		menu.showError("Disconnected:\n" + std::string(e.what()));
		menu.reset();
		joinLobbyClient.reset();
		stopAndJoinServer(lobbyServer, serverThread);
	}
}

// ---------------------------------------------------------
// MAIN
// ---------------------------------------------------------
int main()
{
	auto clientLogger = createConsoleAndFileLogger("Client");
	auto serverLogger = createConsoleAndFileLogger("Server");

	SPDLOG_LOGGER_INFO(clientLogger, "Tank Game Application Starting...");

	sf::RenderWindow window(sf::VideoMode(WINDOW_DIM), "Tank Game", sf::Style::Titlebar | sf::Style::Close);
	window.setFramerateLimit(60);

	Menu menu(WINDOW_DIM);
	sf::Music &menuMusic = initMusic("audio/menu_loop.ogg");

	std::unique_ptr<LobbyServer> lobbyServer;
	std::unique_ptr<std::thread> serverThread;

	std::unique_ptr<LobbyClient> joinLobbyClient;
	bool clientReady = false;

	while(window.isOpen())
	{
		handleMenuEvents(window, menu);
		updateMenuMusic(menu, menuMusic);

		if(menu.shouldExit())
		{
			window.close();
			break;
		}

		if(menu.shouldHostGame())
			handleHostGame(menu, lobbyServer, serverThread, joinLobbyClient, clientLogger, serverLogger);

		if(menu.shouldConnect() && menu.getState() == Menu::State::JOIN_LOBBY && !joinLobbyClient)
			handleJoinGame(menu, joinLobbyClient, clientLogger);

		handleLobbyCleanup(menu, joinLobbyClient, lobbyServer, serverThread, clientReady, clientLogger);
		handleLobbyClientUpdates(window, menu, menuMusic, joinLobbyClient, lobbyServer, serverThread, clientReady,
		                         clientLogger);

		if(menu.shouldStartGame() && joinLobbyClient && !clientReady)
		{
			joinLobbyClient->sendReady();
			clientReady = true;
			menu.disableReadyButton();
		}

		window.clear(sf::Color(20, 20, 20));
		menu.draw(window);
		window.display();
	}

	SPDLOG_LOGGER_INFO(spdlog::get("Client"), "Tank Game shutting down");
	stopAndJoinServer(lobbyServer, serverThread);
	return 0;
}
