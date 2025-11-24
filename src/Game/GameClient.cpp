#include "GameClient.h"

#include <spdlog/spdlog.h>

#include "Utilities.h"

GameClient::GameClient(WorldClient &world, LobbyClient &lobby, const std::shared_ptr<spdlog::logger> &logger)
	: m_world(world), m_lobby(lobby), m_logger(logger),
	  // TODO, the server needs to send its udp port to the clients, or take control over that away from the users
	  m_gameServer({lobby.m_lobbySock.getRemoteAddress().value(), PORT_UDP})
{
}

GameClient::~GameClient() = default;

void GameClient::handleUserInputs(sf::RenderWindow &window)
{

	if(m_showEndscreen && m_endscreen){
        float dt = m_endscreenClock.restart().asSeconds();
        m_endscreen->update(dt);
    }
		
	if(m_showEndscreen && m_endscreen && m_endscreen->shouldExit())
	{
		m_endscreen.reset();
		m_showEndscreen = false;
	}

	std::optional<sf::Packet> outPktOpt = m_world.update();
	m_world.draw(window);

	if(m_showEndscreen && m_endscreen)
		m_endscreen->draw(window);

	window.display();

	if(outPktOpt.has_value())
	{
		if(checkedSend(m_lobby.m_gameSock, outPktOpt.value(), m_gameServer.ip, m_gameServer.port) !=
		   sf::Socket::Status::Done)
			SPDLOG_LOGGER_ERROR(m_logger, "UDP send failed");
	}
}

bool GameClient::processReliablePackets(sf::TcpSocket &lobbySock, sf::RenderWindow &window)
{
	sf::Packet reliablePkt;
	if(checkedReceive(lobbySock, reliablePkt) == sf::Socket::Status::Done)
	{
		uint8_t type;
		reliablePkt >> type;

		if(type == uint8_t(ReliablePktType::GAME_END))
		{
			EntityId winnerId;
			reliablePkt >> winnerId;

			// Anzahl Spieler
			uint32_t numPlayers;
			reliablePkt >> numPlayers;

			// Kills/Deaths in die lokalen PlayerState-Objekte übernehmen
			for(uint32_t i = 0; i < numPlayers; ++i)
			{
				uint32_t playerId;
				uint32_t kills, deaths;
				reliablePkt >> playerId >> kills >> deaths;

				// PlayerState im Client-World finden und aktualisieren
				auto *player = m_world.getPlayerById(playerId);
				if(player)
				{
					player->setKills(kills);
					player->setDeaths(deaths);
				}
			}

			// Jetzt Endscreen anzeigen
			auto winner = m_world.getWinner();
			auto players = m_world.getPlayers();

            m_endscreenClock.restart();
			m_showEndscreen = true;
			m_endscreen = std::make_unique<Endscreen>(window.getSize());
			m_endscreen->setup(*winner, players);

			SPDLOG_LOGGER_INFO(m_logger, "Battle is over, winner id {}", winnerId);
		}
	}
	return false;
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
		// TODO: uncomment
		/*SPDLOG_LOGGER_INFO(m_logger, "Applied snapshot: ({:.0f},{:.0f}), {:.0f}°, hp={}", ops.m_pos.x, ops.m_pos.y,
		                   ops.m_rot.asDegrees(), ops.m_health);*/
	}
	else if(st != sf::Socket::Status::NotReady)
		SPDLOG_LOGGER_ERROR(m_logger, "Failed receiving snapshot pkt: {}", (int)st);
}
