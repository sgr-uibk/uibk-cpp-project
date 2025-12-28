#include "GameClient.h"

#include <spdlog/spdlog.h>

#include "Utilities.h"
#include "Lobby/LobbyClient.h"

GameClient::GameClient(WorldClient &world, LobbyClient &lobby)
	: m_world(world), m_lobby(lobby), m_gameServer({lobby.m_lobbySock.getRemoteAddress().value(), PORT_UDP})
{
}

GameClient::~GameClient() = default;

void GameClient::update(sf::RenderWindow &window) const
{
	using namespace sf::Keyboard;
	using namespace sf::Mouse;
	m_world.pollEvents();
	m_world.m_interp.interpolateEnemies();
	WorldUpdateData wud{};
	if(!window.hasFocus())
	{
		m_world.update(wud);
	END:
		m_world.draw(window);
		window.display();
		return;
	}

	bool const w = isKeyPressed(Scan::W);
	bool const s = isKeyPressed(Scan::S);
	bool const a = isKeyPressed(Scan::A);
	bool const d = isKeyPressed(Scan::D);
	wud.posDelta = sf::Vector2f{float(d - a), float(s - w)};
	wud.bShoot = isKeyPressed(Scan::Space) || isButtonPressed(Button::Left);

	if(m_world.update(wud))
	{
		sf::Packet pkt = createTickedPkt(UnreliablePktType::INPUT, m_world.m_clientTick);
		pkt << wud;
		if(checkedSend(m_lobby.m_gameSock, pkt, m_gameServer.ip, m_gameServer.port) != sf::Socket::Status::Done)
			SPDLOG_LOGGER_ERROR(spdlog::get("Client"), "UDP send failed");
	}
	goto END;
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
			m_world.m_interp.correctLocalPlayer();
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
			// TODO save the player names somewhere, so that we can print the winner here
			if(m_lobby.m_clientId == winnerId)
				SPDLOG_LOGGER_INFO(spdlog::get("Client"), "I won the game!", winnerId);
			else
				SPDLOG_LOGGER_INFO(spdlog::get("Client"), "Battle is over, winner id {}, returning to lobby.",
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
