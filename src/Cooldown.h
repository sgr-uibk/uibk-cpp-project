#pragma once
#include <algorithm>

// this is a very simple class, keep it header-only, it's always inlined
class Cooldown
{
  public:
	Cooldown() = default;

	explicit Cooldown(float const seconds) : m_duration(seconds), m_remaining(0)
	{
	}

	[[nodiscard]] float getDuration() const
	{
		return m_duration;
	}

	void setDuration(float seconds)
	{
		m_duration = seconds;
	}

	[[nodiscard]] bool isReady() const
	{
		return m_remaining == 0;
	}

	void update(float dt)
	{
		m_remaining = std::max(m_remaining - dt, 0.f);
	}

	void trigger()
	{
		m_remaining = m_duration;
	}

	void expire()
	{
		m_remaining = 0;
	}

	// Try to trigger; returns true if was ready
	bool try_trigger()
	{
		const bool wasReady = isReady();
		if(wasReady)
			trigger();
		return wasReady;
	}

  private:
	float m_duration;
	float m_remaining;
};