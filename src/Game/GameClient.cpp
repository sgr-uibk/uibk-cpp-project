#include "GameClient.h"

#include <spdlog/spdlog.h>

#include "Utilities.h"

GameClient::GameClient(WorldClient &world, LobbyClient &lobby,
                       const std::shared_ptr<spdlog::logger> &logger)
	: m_world(world),
	  m_lobby(lobby),
	  m_logger(logger),
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
		if(m_lobby.m_gameSock.send(outPktOpt.value(), m_gameServer.ip, m_gameServer.port) != sf::Socket::Status::Done)
			SPDLOG_LOGGER_ERROR(m_logger, "UDP send failed");
	}
}

void GameClient::syncFromServer() const
{
	sf::Packet snapPkt;
	std::optional<sf::IpAddress> srcAddrOpt;
	uint16_t srcPort;
	if(m_lobby.m_gameSock.receive(snapPkt, srcAddrOpt, srcPort) == sf::Socket::Status::Done)
	{
		expectPkt(snapPkt, UnreliablePktType::SNAPSHOT);
		WorldState snapshot{WINDOW_DIMf};
		snapshot.deserialize(snapPkt);
		m_world.applyServerSnapshot(snapshot);
		auto ops = m_world.getOwnPlayerState();
		SPDLOG_LOGGER_INFO(m_logger, "Applied snapshot: ({:.0f},{:.0f}), {:.0f}°, hp={}",
		                   ops.m_pos.x, ops.m_pos.y, ops.m_rot.asDegrees(), ops.m_health);
	}
}

// returns whether the game ends
bool GameClient::processReliablePackets(sf::TcpSocket &lobbySock) const
{
	sf::Packet reliablePkt;
	if(lobbySock.receive(reliablePkt) != sf::Socket::Status::Done)
	{
		uint8_t type;
		EntityId winnerId;
		reliablePkt >> type;
		reliablePkt >> winnerId;
		switch(type)
		{
		case uint8_t(ReliablePktType::GAME_END): {
			// TODO save the player names somewhere, so that we can print the winner here
			SPDLOG_LOGGER_INFO(m_logger, "Battle is over, winner id {}, returning to lobby.",
			                   winnerId);
			return true;
		}
		default: {
		}
			//assert(type && type < uint8_t(ReliablePktType::_size));
			//SPDLOG_LOGGER_WARN(m_logger, "Unhandled reliable packet type: {}.", type);
		}
	}
	return false;
}
