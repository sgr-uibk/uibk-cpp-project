#include "GameClient.h"

#include <spdlog/spdlog.h>

#include "Utilities.h"
#include "Lobby/LobbyClient.h"
#include "../UI/Scoreboard.h"

GameClient::GameClient(WorldClient &world, LobbyClient &lobby)
	: m_world(world), m_lobby(lobby), m_gameServer({lobby.m_lobbySock.getRemoteAddress().value(), PORT_UDP})
// TODO, the server needs to send its udp port to the clients, or take control over that away from the users
{
}

GameClient::~GameClient() = default;

void GameClient::update(sf::RenderWindow &window) const
{
	m_world.pollEvents();
	m_world.m_interp.interpolateEnemies();
	sf::Vector2f posDelta{0.f, 0.f};

	if(window.hasFocus())
	{
		bool const w = sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::W);
		bool const s = sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::S);
		bool const a = sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::A);
		bool const d = sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::D);
		posDelta = {static_cast<float>(d - a), static_cast<float>(s - w)};
	}
	std::optional<sf::Packet> outPktOpt = m_world.update(posDelta);
	m_world.draw(window);
	window.display();

	if(outPktOpt.has_value())
	{
		// WorldClient.update() produces tick-rate-limited packets, so now is the right time to send.
		if(checkedSend(m_lobby.m_gameSock, outPktOpt.value(), m_gameServer.ip, m_gameServer.port) !=
		   sf::Socket::Status::Done)
			SPDLOG_LOGGER_ERROR(spdlog::get("Client"), "UDP send failed");
	}
}

void GameClient::processUnreliablePackets()
{
	sf::Packet snapPkt;
	std::optional<sf::IpAddress> srcAddrOpt;
	uint16_t srcPort;
	if(sf::Socket::Status st = checkedReceive(m_lobby.m_gameSock, snapPkt, srcAddrOpt, srcPort);
	   st == sf::Socket::Status::Done)
	{
		Tick const authTick = expectTickedPkt(snapPkt, UnreliablePktType::SNAPSHOT);
		WorldState const worldSnap(snapPkt);

		if(m_world.m_interp.storeSnapshot(authTick, std::move(snapPkt), worldSnap))
		{ // The received state is new and can be applied
			m_world.m_interp.correctLocalPlayer(worldSnap);
			m_world.applyNonInterpState(worldSnap);
		}
	}
	else if(st != sf::Socket::Status::NotReady)
		SPDLOG_LOGGER_ERROR(spdlog::get("Client"), "Failed receiving snapshot pkt: {}", (int)st);
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

			std::vector<Scoreboard::PlayerStats> playerStats;
			playerStats.reserve(numPlayers);

			for(uint32_t i = 0; i < numPlayers; ++i)
			{
				Scoreboard::PlayerStats stats;
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
				SPDLOG_LOGGER_INFO(spdlog::get("Client"), "I won the game!", winnerId);
			else
				SPDLOG_LOGGER_INFO(spdlog::get("Client"), "Battle is over, winner id {}, scoreboard displayed.",
				                   winnerId);

			return true;
		}
		default:
			SPDLOG_LOGGER_WARN(spdlog::get("Client"), "Unhandled reliable packet type: {}, size={}", type,
			                   reliablePkt.getDataSize());
		}
	}
	return false;
}
