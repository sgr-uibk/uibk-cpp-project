#include "InterpClient.h"
#include "Utilities.h"
#include "World/WorldClient.h"

InterpClient::InterpClient(MapState const &map, EntityId const id, std::array<PlayerClient, MAX_PLAYERS> &players)
	: m_map(map), m_id(id), m_players(players), m_stateBuffer(Ticked(0, extractStates(players)))
{
}

void InterpClient::predictMovement(sf::Vector2f const posDelta)
{
	m_inputAcc += posDelta;
	m_player.applyLocalMove(m_map, posDelta);
}

/// \brief After a correction from the server, local inputs need to be replayed
void InterpClient::replayInputs(PlayerState &player)
{
	for(size_t i = m_inflightInputs.capacity(); i > 0; --i)
	{
		auto &delta = m_inflightInputs.get(i - 1);
		if(delta.tick > m_ackedTick)
			player.moveOn(m_map, delta.obj); // replay in flight
	}
	player.moveOn(m_map, m_inputAcc); // replay pre-in flight inputs
}

void InterpClient::replayInputs(PlayerClient &player)
{
	replayInputs(player.m_state);
	player.syncSpriteToState();
}

/// \brief Call once a server-tick to release accumulated inputs for sending
sf::Vector2f InterpClient::getCumulativeInputs(Tick clientTick)
{
	auto const acc = m_inputAcc;
	m_inputAcc = {0, 0}; // now in flight
	Tick const oldest = m_stateBuffer.get(-1).tick;
	if(m_ackedTick < oldest)
		SPDLOG_LOGGER_WARN(spdlog::get("Client"),
		                   "Inflight input buffer discontinuity: {} overrides {} (last acked {})", clientTick, oldest, m_ackedTick);
	m_inflightInputs.emplace(clientTick, acc);
	return acc;
}

bool InterpClient::storeSnapshot(Tick authTick, sf::Packet snapPkt, WorldState const &ws)
{
	Tick latestKnown = m_stateBuffer.get().tick;
	if(latestKnown >= authTick)
	{
		SPDLOG_LOGGER_WARN(spdlog::get("Client"), "Ignoring outdated snapshot created at server tick #{} (know {})",
		                   authTick, latestKnown);
		return false;
	}

	std::array<Tick, MAX_PLAYERS> ackedTicks{};
	snapPkt >> ackedTicks;
	m_ackedTick = ackedTicks[m_id - 1];
	m_stateBuffer.emplace(authTick, ws.m_players);
	return true;
}

void InterpClient::correctLocalPlayer()
{
	PlayerState authState = m_stateBuffer.get().obj[m_id - 1];
	PlayerState localState = m_player.getState();

	// Would we end up with the same state when applying inputs to the authoritative state ?
	replayInputs(authState);
	auto err = authState.m_iState.m_pos - localState.m_iState.m_pos;
	if(err.lengthSquared() <= 1e-3)
	{ // Prediction hit! We're done here.
		return;
	}

	// Server disagrees, have to accept authoritative state, replay local inputs
	SPDLOG_LOGGER_WARN(spdlog::get("Client"), "Prediction miss", err.x, err.y);
	m_player.overwriteInterpState(authState.m_iState);
	replayInputs(localState);

#ifdef SFML_DEBUG
	// Print how much the teleport is, that the player is going to see
	auto const replayedState = m_player.getState().m_iState;
	SPDLOG_LOGGER_WARN(spdlog::get("Client"), "Prediction error ({:.2f},{:.2f})", err.x, err.y);
	err = replayedState.m_pos - localState.m_iState.m_pos;
	if(err.lengthSquared() > 1e-3)
		SPDLOG_LOGGER_WARN(spdlog::get("Client"), "Reconciliation error ({:.2f},{:.2f})", err.x, err.y);
#endif
}

/// \note call every render frame
void InterpClient::interpolateEnemies()
{
	// Render‑time interpolation
	long const renderTick = m_stateBuffer.get().tick - 1;
	if(renderTick < 0)
		return;

	struct InterpState
	{
		size_t step = size_t(-1);
		Ticked<std::array<PlayerState, MAX_PLAYERS>> const *ss0 = nullptr;
		Ticked<std::array<PlayerState, MAX_PLAYERS>> const *ss1 = nullptr;
	};
	static InterpState s;

	if(s.step >= RENDER_TICK_HZ / UNRELIABLE_TICK_HZ)
	{ // completed interpolation, get the next pair
		s = {.step = 0, .ss0 = &m_stateBuffer.get(1), .ss1 = &m_stateBuffer.get(0)};
	}

	if(s.ss0->tick + 1 != s.ss1->tick && s.ss0->tick && s.ss1->tick)
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("Client"), "SS buffer out of sequence: {},{}", s.ss0->tick, s.ss1->tick);
		return;
	}

	float const alpha = s.step++ * UNRELIABLE_TICK_HZ / float(RENDER_TICK_HZ);

	for(size_t i = 0; i < MAX_PLAYERS; ++i)
	{
		auto &p0 = s.ss0->obj[i];
		auto &p1 = s.ss1->obj[i];
		if(i + 1 == m_id)
			continue; // own player is predicted, not interpolated

		m_players[i].interp(p0.m_iState, p1.m_iState, alpha);
	}
}
