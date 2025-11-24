#include "GameClient.h"

#include <spdlog/spdlog.h>

#include "Utilities.h"
#include "Lobby/LobbyClient.h"

GameClient::GameClient(WorldClient &world, LobbyClient &lobby, const std::shared_ptr<spdlog::logger> &logger)
	: m_world(world), m_lobby(lobby), m_logger(logger),
	  // TODO, the server needs to send its udp port to the clients, or take control over that away from the users
	  m_gameServer({lobby.m_lobbySock.getRemoteAddress().value(), PORT_UDP})
{
}

GameClient::~GameClient() = default;

void GameClient::handleUserInputs(sf::RenderWindow &window) const
{
	m_world.pollEvents();
	std::optional<sf::Packet> outPktOpt = m_world.update();
	m_world.draw(window);
	window.display();

	if(outPktOpt.has_value())
	{
		// WorldClient.update() produces tick-rate-limited packets, so now is the right time to send.
		if(checkedSend(m_lobby.m_gameSock, outPktOpt.value(), m_gameServer.ip, m_gameServer.port) !=
		   sf::Socket::Status::Done)
			SPDLOG_LOGGER_ERROR(m_logger, "UDP send failed");
	}
}

void GameClient::syncFromServer() const
{
	sf::Packet snapPkt;
	std::optional<sf::IpAddress> srcAddrOpt;
	uint16_t srcPort;
	if(sf::Socket::Status st = checkedReceive(m_lobby.m_gameSock, snapPkt, srcAddrOpt, srcPort);
	   st == sf::Socket::Status::Done)
	{
		expectPkt(snapPkt, UnreliablePktType::SNAPSHOT);
		WorldState snapshot{WINDOW_DIMf};
		snapshot.deserialize(snapPkt);
		m_world.applyServerSnapshot(snapshot);
		auto ops = m_world.getOwnPlayerState();
		SPDLOG_LOGGER_INFO(m_logger, "Applied snapshot: ({:.0f},{:.0f}), {:.0f}°, hp={}", ops.m_pos.x, ops.m_pos.y,
		                   ops.m_rot.asDegrees(), ops.m_health);
	}
	else if(st != sf::Socket::Status::NotReady)
		SPDLOG_LOGGER_ERROR(m_logger, "Failed receiving snapshot pkt: {}", (int)st);
}

// returns whether the game ends
bool GameClient::processReliablePackets(sf::TcpSocket &lobbySock) const
{
	sf::Packet reliablePkt;
	if(checkedReceive(lobbySock, reliablePkt) == sf::Socket::Status::Done)
	{
		uint8_t type;
		reliablePkt >> type;

		switch(type)
		{
		case uint8_t(ReliablePktType::GAME_END): {
			EntityId winnerId;
			reliablePkt >> winnerId;

			// Read player stats
			uint32_t numPlayers;
			reliablePkt >> numPlayers;

			std::vector<WorldClient::PlayerStats> playerStats;
			playerStats.reserve(numPlayers);

			for(uint32_t i = 0; i < numPlayers; ++i)
			{
				WorldClient::PlayerStats stats;
				reliablePkt >> stats.id;
				reliablePkt >> stats.name;

				int32_t kills, deaths;
				reliablePkt >> kills >> deaths;
				stats.kills = kills;
				stats.deaths = deaths;

				playerStats.push_back(stats);
			}

			// Show the scoreboard UI with player stats
			m_world.showScoreboard(winnerId, playerStats);

			if(m_lobby.m_clientId == winnerId)
				SPDLOG_LOGGER_INFO(m_logger, "I won the game!", winnerId);
			else
				SPDLOG_LOGGER_INFO(m_logger, "Battle is over, winner id {}, scoreboard displayed.", winnerId);

			return false;
		}
		case uint8_t(ReliablePktType::SERVER_SHUTDOWN): {
			SPDLOG_LOGGER_WARN(m_logger, "Server shutdown during game");
			throw ServerShutdownException();
		}
		case uint8_t(ReliablePktType::LOBBY_UPDATE): {
			// Server is broadcasting lobby updates (after game ends when resetLobbyState is called)
			// We need to parse and update the lobby client's state so it's fresh when we return
			uint32_t numPlayers;
			reliablePkt >> numPlayers;

			// Parse all player data
			std::vector<LobbyPlayerInfo> players;
			for(uint32_t i = 0; i < numPlayers; ++i)
			{
				LobbyPlayerInfo playerInfo;
				reliablePkt >> playerInfo.id >> playerInfo.name >> playerInfo.bReady;
				players.push_back(playerInfo);
			}

			// Update the lobby client's player list
			m_lobby.updateLobbyPlayers(players);

			SPDLOG_LOGGER_DEBUG(m_logger, "Received LOBBY_UPDATE during game, updated {} players", numPlayers);
			return false;
		}
		default:
			SPDLOG_LOGGER_WARN(m_logger, "Unhandled reliable packet type: {}, size={}", type,
			                   reliablePkt.getDataSize());
		}
	}
	return false;
}
