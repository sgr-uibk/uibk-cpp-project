#pragma once
#include <array>
#include <mutex>
#include <optional>

// Unidirectional ring buffer implementation (think cyclic FIFO structure)
template <class T, size_t Sz> struct RingQueue
{
	constexpr RingQueue() : m_buf{}
	{
	}

	template <typename... U, typename = std::enable_if_t<sizeof...(U) == Sz>>
	explicit constexpr RingQueue(U &&...u) : m_buf{T(std::forward<U>(u))...}
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

	/// \Returns the most recently inserted element.
	T &get() const noexcept
	{
		return m_buf[m_hot % Sz];
	}

	/// \Returns the element that was inserted \param k positions before the most recent one
	T &get(std::size_t k) const noexcept
	{
		return m_buf[(m_hot - k) % Sz];
	}

	/// \brief Marks the buffer as empty
	void clear() noexcept
	{
		m_hot = Sz - 1;
	}

	size_t capacity() noexcept
	{
		return Sz;
	}

	std::size_t m_hot = 0;
	mutable std::array<T, Sz> m_buf;
};

// If Sz is a power of 2, the compiler should optimize the wrap-around to a simple bitwise &,
// instead of a division with remainder.
template <class T, size_t Sz> class RingBuffer
{
  public:
	RingBuffer() = default;
	RingBuffer(std::initializer_list<T> initList)
	{
		static_assert(initList.size() == Sz, "Initializer list size must match buffer size");
		for(size_t i = 0; i < initList.size(); ++i)
		{
			m_buf[i] = initList[i];
		}
	}

	bool push(T thing)
	{
		std::lock_guard lock(m_mutex);
		bool bSuccess = !isEmpty();

		if(bSuccess)
		{
			m_buf[m_out] = thing;
			m_out = (m_out + 1) % Sz;
		}
		return bSuccess;
	}

	std::optional<T &> get() noexcept
	{
		std::lock_guard lock(m_mutex);

		if(isEmpty())
			return std::nullopt;

		auto &ref = m_buf[m_out];
		m_out = (m_out + 1) % Sz;
		return ref;
	}

	void reset() const noexcept
	{
		std::lock_guard lock(m_mutex);
		m_in = m_out;
	}

	bool isEmpty() const noexcept
	{
		return m_in == m_out;
	}

	bool isFull() const noexcept
	{
		// If tail is ahead the head by 1, we are full
		auto tailNext = (m_out + 1) % Sz;
		return tailNext == m_in;
	}

	bool safeIsEmpty() const noexcept
	{
		std::lock_guard lock(m_mutex);
		return isEmpty();
	}

	bool safeIsFull() const noexcept
	{
		std::lock_guard lock(m_mutex);
		return isFull();
	}

	size_t getCapacity() const noexcept
	{
		return Sz;
	}

	size_t size() const noexcept
	{
		std::lock_guard lock(m_mutex);

		intmax_t size = m_in - m_out;

		// if(!isFull())
		{
			size += (size < 0) * Sz;
		}

		return size;
	}

	mutable std::recursive_mutex m_mutex;
	mutable std::array<T, Sz> m_buf;
	mutable size_t m_in = 0;
	mutable size_t m_out = 0;
};
