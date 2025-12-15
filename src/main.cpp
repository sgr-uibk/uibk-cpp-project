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
#include "World/WorldClient.h"

void runGameLoop(sf::RenderWindow &window, LobbyClient &lobbyClient, int mapIndex,
                 std::array<PlayerState, MAX_PLAYERS> const &players)
{
	SPDLOG_LOGGER_INFO(spdlog::get("Client"), "Starting game with player ID {} on map {}", lobbyClient.m_clientId,
	                   mapIndex);

	lobbyClient.m_lobbySock.setBlocking(false);

	int validPlayerCount = 0;
	for(size_t i = 0; i < players.size(); ++i)
	{
		if(players[i].m_id != 0)
		{
			validPlayerCount++;
			SPDLOG_LOGGER_INFO(spdlog::get("Client"), "Player {}: pos=({:.1f},{:.1f}), rot={:.1f}°, hp={}",
			                   players[i].m_id, players[i].getPosition().x, players[i].getPosition().y,
			                   players[i].getRotation().asDegrees(), players[i].getHealth());
		}
	}
	SPDLOG_LOGGER_INFO(spdlog::get("Client"), "Starting game loop with {} valid players", validPlayerCount);
	WorldClient worldClient(window, lobbyClient.m_clientId, mapIndex, players);
	GameClient gameClient(worldClient, lobbyClient);

	while(window.isOpen())
	{
		gameClient.update(window);

		if(worldClient.m_pauseMenu.isDisconnectRequested())
		{
			SPDLOG_LOGGER_INFO(spdlog::get("Client"), "Player disconnected via pause menu, returning to lobby...");
			break;
		}

		gameClient.processUnreliablePackets();
		if(gameClient.processReliablePackets(lobbyClient.m_lobbySock))
		{
			SPDLOG_LOGGER_INFO(spdlog::get("Client"), "Game ended, returning to lobby...");
			break;
		}
	}

	lobbyClient.m_lobbySock.setBlocking(true);

	SPDLOG_LOGGER_INFO(spdlog::get("Client"), "Game window closed, returning to lobby...");
}

int main()
{
	auto logger = createConsoleAndFileLogger("Client");
	SPDLOG_LOGGER_INFO(spdlog::get("Client"), "Tank Game starting...");

	sf::RenderWindow window(sf::VideoMode(WINDOW_DIM), "Tank Game", sf::Style::Titlebar | sf::Style::Close);
	window.setFramerateLimit(60);

	Menu menu(WINDOW_DIM);
	sf::Music &menuMusic = initMusic("audio/menu_loop.ogg");
	std::unique_ptr<LobbyClient> lobbyClient;
	bool clientReady = false;

	while(window.isOpen())
	{
		while(std::optional const event = window.pollEvent())
		{
			if(event->is<sf::Event::Closed>())
				window.close();
			else if(auto const *mouseButtonPressed = event->getIf<sf::Event::MouseButtonPressed>())
			{
				if(mouseButtonPressed->button == sf::Mouse::Button::Left)
					menu.handleClick(window.mapPixelToCoords(mouseButtonPressed->position, window.getView()));
			}
			else if(auto const *mouseMoved = event->getIf<sf::Event::MouseMoved>())
				menu.handleMouseMove(window.mapPixelToCoords(mouseMoved->position, window.getView()));
			else if(auto const *textEntered = event->getIf<sf::Event::TextEntered>())
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

		if(menu.shouldConnect() && menu.getState() == Menu::State::JOIN_LOBBY && !lobbyClient)
		{
			SPDLOG_LOGGER_INFO(spdlog::get("Client"), "Connecting to game at {}:{} as player '{}'", menu.getServerIp(),
			                   menu.getServerPort(), menu.getPlayerName());

			try
			{
				auto ipAddress = sf::IpAddress::resolve(menu.getServerIp());
				if(!ipAddress.has_value())
				{
					throw std::runtime_error("Failed to resolve IP address: " + menu.getServerIp());
				}

				lobbyClient = std::make_unique<LobbyClient>(menu.getPlayerName(),
				                                            Endpoint{ipAddress.value(), menu.getServerPort()});
				lobbyClient->connect();

				menu.setState(Menu::State::LOBBY_CLIENT);
				menu.setTitle("IN LOBBY");
				menu.setupLobbyClient();

				SPDLOG_LOGGER_INFO(spdlog::get("Client"), "Connected to server, joined lobby");
			}
			catch(std::exception const &e)
			{
				SPDLOG_LOGGER_ERROR(spdlog::get("Client"), "Failed to connect to server: {}", e.what());
				lobbyClient.reset();
			}
			menu.clearConnectFlag();
		}

		// reset join lobby client if we left the lobby client screen
		if(menu.getState() != Menu::State::LOBBY_CLIENT && lobbyClient)
		{
			SPDLOG_LOGGER_INFO(spdlog::get("Client"), "Left lobby client screen, disconnecting...");
			lobbyClient.reset();
			clientReady = false;
		}

		if(menu.getState() == Menu::State::LOBBY_CLIENT && lobbyClient)
		{
			try
			{
				auto gameStartResult = lobbyClient->waitForGameStart(sf::milliseconds(100));
				if(gameStartResult.has_value())
				{
					SPDLOG_LOGGER_INFO(spdlog::get("Client"), "Client received GAME_START packet, starting game...");

					menuMusic.stop();
					auto const &[mapIndex, players] = *gameStartResult;
					runGameLoop(window, *lobbyClient, mapIndex, players);
					clientReady = false;
				}
				else
				{
					lobbyClient->pollLobbyUpdate();
					menu.updateLobbyDisplay(lobbyClient->getLobbyPlayers());
					menu.updateClientButton(clientReady);
				}
			}
			catch(ServerShutdownException const &e)
			{
				SPDLOG_LOGGER_WARN(spdlog::get("Client"), "Server has shut down: {}", e.what());
				lobbyClient.reset();
				clientReady = false;
				menu.reset();
			}
		}

		if(menu.shouldStartGame() && menu.getState() == Menu::State::LOBBY_CLIENT && lobbyClient)
		{
			if(!clientReady)
			{
				SPDLOG_LOGGER_INFO(spdlog::get("Client"), "Client marked as ready");
				lobbyClient->sendReady();
				clientReady = true;
			}
			menu.clearStartGameFlag();
		}

		window.clear(sf::Color(20, 20, 20));
		menu.draw(window);
		window.display();
	}

	SPDLOG_LOGGER_INFO(spdlog::get("Client"), "Tank Game shutting down");
	return 0;
}
