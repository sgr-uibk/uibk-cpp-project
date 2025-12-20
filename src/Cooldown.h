#pragma once
#include <algorithm>

// this is a very simple class, keep it header-only, it's always inlined
class Cooldown
{
  public:
	Cooldown() = default;

	[[nodiscard]] bool isReady() const
	{
		return m_remaining == 0;
	}

	void update(float dt)
	{
		m_remaining = std::max(m_remaining - dt, 0.f);
	}

	void trigger(float const duration)
	{
		m_remaining = duration;
	}

	void expire()
	{
		m_remaining = 0;
	}

	// Try to trigger; returns true if was ready
	bool try_trigger(float duration)
	{
		bool const wasReady = isReady();
		if(wasReady)
			trigger(duration);
		return wasReady;
	}

	[[nodiscard]] float getRemaining() const
	{
		return m_remaining;
	}

	void setRemaining(float seconds)
	{
		m_remaining = seconds;
	}

  private:
	float m_remaining;
};
