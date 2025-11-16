#pragma once
#include <array>
#include <mutex>
#include <optional>

// Unidirectional ring buffer implementation (think cyclic FIFO structure)
template <class T, size_t Sz> struct RingQueue
{
	constexpr RingQueue() : m_hot(0), m_buf{}
	{
	}

	void push(T val) noexcept
	{
		m_buf[++m_hot % Sz] = val;
	}

	T &claim() noexcept
	{
		return m_buf[++m_hot % Sz];
	}

	T &get() const noexcept
	{
		return m_buf[m_hot % Sz];
	}

	size_t m_hot;
	mutable std::array<T, Sz> m_buf;
};
