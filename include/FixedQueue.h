/*
* Vool - Fixed queue on top of std::array
*
* Copyright (c) 2016 Lukas Bergdoll - www.lukas-bergdoll.net
*
* This code is licensed under the Apache License 2.0 (https://opensource.org/licenses/Apache-2.0)
*/

#ifndef VOOL_FIXEDQUEUE_H_INCLUDED
#define VOOL_FIXEDQUEUE_H_INCLUDED

#include <array>
#include <type_traits>

namespace vool
{

template<typename T, size_t N> class fixed_queue
{
public:
	using buffer_t = std::array<T, N>;

	explicit fixed_queue() : index_(0)
	{
		static_assert(N > 0, "fixed_queue size should be larger than zero!");
	};

	~fixed_queue() {};

	fixed_queue(const fixed_queue&) = default;
	fixed_queue(fixed_queue&&) = default;

	fixed_queue& operator= (const fixed_queue&) = default;
	fixed_queue& operator= (fixed_queue&&) = default;

	void push_back(const T& val)
		noexcept(std::is_trivially_copy_assignable<T>::value)
	{
		++index_;
		buff_[index_ % N] = val;
	}

	void push_back(T&& val)
		noexcept(std::is_trivially_move_assignable<T>::value)
	{
		++index_;
		buff_[index_ % N] = std::move(val);
	}

	T& front() noexcept { return buff_[index_ % N]; }
	T& back() noexcept { return buff_[(index_ + 1) % N]; }

	template<typename F> void fold(F&& func)
		noexcept(noexcept(func(front())))
	{
		for (size_t i = 0, max = index_ < N ? index_ : N ; i < max; ++i)
			std::forward<F>(func)(buff_[(index_ - i) % N]);
	}
private:
	size_t index_;
	buffer_t buff_;
};

template<typename T, size_t N> T merge_ring_range(
	fixed_queue<T, N>& client_agents
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

#endif // VOOL_FIXEDQUEUE_H_INCLUDED