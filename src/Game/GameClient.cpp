#include "GameClient.h"

#include <spdlog/spdlog.h>

#include "Utilities.h"

GameClient::GameClient(WorldClient &world, LobbyClient &lobby, std::shared_ptr<spdlog::logger> const &logger)
	: m_world(world), m_lobby(lobby), m_gameServer({lobby.m_lobbySock.getRemoteAddress().value(), PORT_UDP}),
	  // TODO, the server needs to send its udp port to the clients, or take control over that away from the users
	  m_logger(logger)
{
}

GameClient::~GameClient() = default;

void GameClient::update(sf::RenderWindow &window) const
{
	m_world.pollEvents();
	m_world.interpolateEnemies();

	bool const w = sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::W);
	bool const s = sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::S);
	bool const a = sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::A);
	bool const d = sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::D);
	sf::Vector2f const posDelta{static_cast<float>(d - a), static_cast<float>(s - w)};
	std::optional<sf::Packet> outPktOpt = m_world.update(posDelta);
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

void GameClient::processUnreliablePackets()
{
	sf::Packet snapPkt;
	std::optional<sf::IpAddress> srcAddrOpt;
	uint16_t srcPort;
	if(sf::Socket::Status st = checkedReceive(m_lobby.m_gameSock, snapPkt, srcAddrOpt, srcPort);
	   st == sf::Socket::Status::Done)
	{
		Tick const authTick = expectTickedPkt(snapPkt, UnreliablePktType::SNAPSHOT);
		std::array<Tick, MAX_PLAYERS> ackedTicks{};
		snapPkt >> ackedTicks;
		[[unlikely]] if(m_world.m_snapshotBuffer.get().tick >= authTick)
		{
			SPDLOG_LOGGER_WARN(m_logger, "Ignoring outdated snapshot created at server tick #{}", authTick);
			return;
		}
		m_world.m_authTick = authTick;
		auto &[snapTick, snapState] = m_world.m_snapshotBuffer.claim();
		snapTick = authTick;
		snapState.deserialize(snapPkt);
		assert(m_world.m_snapshotBuffer.get().tick == authTick);
		m_world.reconcileLocalPlayer(ackedTicks[m_lobby.m_clientId - 1], snapState);
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
			// TODO save the player names somewhere, so that we can print the winner here
			if(m_lobby.m_clientId == winnerId)
				SPDLOG_LOGGER_INFO(m_logger, "I won the game!", winnerId);
			else
				SPDLOG_LOGGER_INFO(m_logger, "Battle is over, winner id {}, returning to lobby.", winnerId);
			m_world.reset();
			return true;
		}
		default:
			SPDLOG_LOGGER_WARN(m_logger, "Unhandled reliable packet type: {}, size={}", type,
			                   reliablePkt.getDataSize());
		}
	}
	return false;
}
