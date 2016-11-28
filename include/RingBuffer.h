/*
* Vool - Ring buffer on top of std::array
*
* Copyright (c) 2016 Lukas Bergdoll - www.lukas-bergdoll.net
*
* This code is licensed under the Apache License 2.0 (https://opensource.org/licenses/Apache-2.0)
*/

#ifndef VOOL_RINGBUFFER_H_INCLUDED
#define VOOL_RINGBUFFER_H_INCLUDED

#include <array>
#include <type_traits>

namespace vool
{

template<typename T, size_t N> class ring_buffer
{
public:
	using buffer_t = std::array<T, N>;
	using iterator = typename buffer_t::iterator;
	using const_iterator = typename buffer_t::const_iterator;

	explicit ring_buffer() : index_(0)
	{
		static_assert(N > 0, "ring_buffer size should be larger than zero!");
	};

	~ring_buffer() {};

	ring_buffer(const ring_buffer&) = default;
	ring_buffer(ring_buffer&&) = default;

	ring_buffer& operator= (const ring_buffer&) = default;
	ring_buffer& operator= (ring_buffer&&) = default;

	void push_back(const T& val)
		noexcept(std::is_trivially_copy_assignable<T>::value)
	{
		buff_[index_ % N] = val;
		++index_;
	}

	void push_back(T&& val)
		noexcept(std::is_trivially_move_assignable<T>::value)
	{
		buff_[index_ % N] = std::move(val);
		++index_;
	}

	T& front() noexcept { return buff_[(index_ - 1) % N]; }
	T& back() noexcept { return buff_[(index_ - N) % N]; }

	iterator begin() noexcept { return buff_.begin(); }
	iterator end() noexcept { return buff_.end(); }

	const_iterator cbegin() const noexcept { return buff_.cbegin(); }
	const_iterator cend() const noexcept { return buff_.cend(); }

	template<typename F> void fold(F&& func)
		noexcept(noexcept(func(front())))
	{
		for (size_t i = 1, max = (index_ < N ? index_ : N) + 1; i < max; ++i)
			std::forward<F>(func)(buff_[(index_ - i) % N]);
	}
private:
	size_t index_;
	buffer_t buff_;
};

template<typename T, size_t N> T merge_ring_range(
	ring_buffer<T, N>& client_agents
)
{
	T ret;

	size_t capacity{};
	client_agents.fold(
		[&capacity](const auto& s) noexcept -> void
		{ capacity += s.capacity(); }
	);
	ret.reserve(capacity);

	client_agents.fold(
		[&ret](const auto& val) -> void { ret += val; }
	);

	return ret;
}

}

#endif // VOOL_RINGBUFFER_H_INCLUDED