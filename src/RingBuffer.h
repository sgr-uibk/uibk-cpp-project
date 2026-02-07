#pragma once
#include "Utilities.h"
#include <array>
#include <mutex>
#include <optional>

// Unidirectional ring buffer implementation (think cyclic FIFO structure)
template <class T, size_t Sz> struct RingQueue
{
	constexpr RingQueue() : m_buf{}
	{
	}

	explicit constexpr RingQueue(T ti) : m_buf{fill<T, Sz>(ti, std::make_index_sequence<Sz>{})}
	{
	}

	/// \brief Inserts a new element, overwriting the oldest entry.
	void push(T val) noexcept
	{
		m_buf[++m_hot % Sz] = val;
	}

	/// \brief Constructs an element in-place, overwriting the oldest entry.
	template <typename... Args> void emplace(Args &&...args) noexcept
	{
		m_buf[++m_hot % Sz] = T(std::forward<Args>(args)...);
	}

	/// \brief Claims a slot for an element to be filled later by the caller
	/// \return Reference to the claimed element slot.
	T &claim() noexcept
	{
		return m_buf[++m_hot % Sz];
	}

	/// \Returns the element that was inserted \param k positions before the most recent one
	T &get(std::size_t const k = 0) const noexcept
	{
		return m_buf[(m_hot - k + Sz) % Sz];
	}

	/// \brief Marks the buffer as empty
	void clear() noexcept
	{
		m_hot = Sz - 1;
	}

	constexpr size_t capacity() noexcept
	{
		return Sz;
	}

	std::size_t m_hot = 0;
	mutable std::array<T, Sz> m_buf;
};
