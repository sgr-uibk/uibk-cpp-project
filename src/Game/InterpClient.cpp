#include "InterpClient.h"
#include "World/WorldClient.h"

InterpClient::InterpClient(MapState const &map, EntityId const id, std::array<PlayerClient, MAX_PLAYERS> &players)
	: m_map(map), m_id(id), m_players(players)
{
	SPDLOG_LOGGER_INFO(spdlog::get("Client"), "interp created, id={}, pl.id={}", m_id, m_player.getState().m_id);
}

void InterpClient::predictMovement(sf::Vector2f posDelta)
{
	m_inputAcc += posDelta;
	m_player.applyLocalMove(m_map, posDelta);
}

/// \brief After a correction from the server, local inputs need to be replayed
void InterpClient::replayInputs()
{
	for(size_t i = m_inflightInputs.capacity(); i > 0; --i)
	{
		auto &delta = m_inflightInputs.get(i - 1);
		if(delta.tick > m_ackedTick)
			m_player.applyLocalMove(m_map, delta.obj); // replay in flight
	}
	m_player.applyLocalMove(m_map, m_inputAcc); // replay pre-in flight inputs
}

/// \brief Call once a server-tick to release accumulated inputs for sending
sf::Vector2f InterpClient::getCumulativeInputs(Tick clientTick)
{
	auto const acc = m_inputAcc;
	m_inputAcc = {0, 0}; // now in flight
	m_inflightInputs.emplace(clientTick, acc);
	return acc;
}

WorldState *InterpClient::storeSnapshot(Tick authTick, sf::Packet snapPkt)
{
	Tick latestKnown = m_snapshotBuffer.get().tick;
	if(latestKnown >= authTick)
	{
		SPDLOG_LOGGER_WARN(spdlog::get("Client"), "Ignoring outdated snapshot created at server tick #{} (know {})",
		                   authTick, latestKnown);
		return nullptr;
	}

	std::array<Tick, MAX_PLAYERS> ackedTicks{};
	snapPkt >> ackedTicks;
	m_ackedTick = ackedTicks[m_id - 1];

	auto &[snapTick, snapState] = m_snapshotBuffer.claim();
	snapTick = authTick;
	snapState.deserialize(snapPkt);

	assert(m_snapshotBuffer.get().tick == authTick);
	return &snapState;
}

void InterpClient::syncLocalPlayer(WorldState const &snap)
{
	PlayerState authState = snap.m_players[m_id - 1];
	PlayerState localState = m_player.getState();

	auto err = authState.m_pos - localState.m_pos;
	if(err.lengthSquared() == 0)
	{ // Prediction hit! We're done here.
		return;
	}

	// Server disagrees, have to accept authoritative state, replay local inputs
	m_player.applyServerState(authState);
	replayInputs();

#ifdef SFML_DEBUG
	// Print how much the teleport is, that the player is going to see
	auto const replayedState = m_player.getState();
	err = replayedState.m_pos - localState.m_pos;
	if(err.lengthSquared() > 1e-3)
		SPDLOG_LOGGER_WARN(spdlog::get("Client"), "Reconciliation error ({},{})", err.x, err.y);
#endif
}

/// \note call every render frame
void InterpClient::interpolateEnemies()
{
	// Render‑time interpolation
	long const renderTick = m_snapshotBuffer.get().tick - 1;
	if(renderTick < 0)
	{
		SPDLOG_LOGGER_INFO(spdlog::get("Client"), "Interpolation not yet ready");
		return;
	}

	struct InterpState
	{
		size_t step = size_t(-1);
		Ticked<WorldState> const *ss0 = nullptr;
		Ticked<WorldState> const *ss1 = nullptr;
	};
	static InterpState s;

	if(s.step >= RENDER_TICK_HZ / UNRELIABLE_TICK_HZ)
	{ // completed interpolation, get the next pair
		s = {.step = 0, .ss0 = &m_snapshotBuffer.get(1), .ss1 = &m_snapshotBuffer.get(0)};
	}

	if(s.ss0->tick + 1 != s.ss1->tick && s.ss0->tick && s.ss1->tick)
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("Client"), "SS buffer out of sequence: {},{}", s.ss0->tick, s.ss1->tick);
		return;
	}

	float const alpha = s.step++ * UNRELIABLE_TICK_HZ / float(RENDER_TICK_HZ);

	for(size_t i = 0; i < MAX_PLAYERS; ++i)
	{
		auto &p0 = s.ss0->obj.m_players[i];
		auto &p1 = s.ss1->obj.m_players[i];
		if(i + 1 == m_id)
			continue; // own player is predicted, not interpolated

		m_players[i].interp(p0, p1, alpha);
	}
}
