/*
* Vool - Unit tests for ring_buffer
*
* Copyright (c) 2016 Lukas Bergdoll - www.lukas-bergdoll.net
*
* This code is licensed under the Apache License 2.0 (https://opensource.org/licenses/Apache-2.0)
*/

#include "AllTests.h"

#include <RingBuffer.h>

#include <string>

namespace vool
{
namespace tests
{

template<typename T, size_t N> void test_impl(T val_a, T val_b)
{
	ring_buffer<T, N> rb;

	rb.push_back(val_b);
	for (size_t i = 0; i < N-1; ++i)
		rb.push_back(val_a);
	
	if (rb.back() != val_b)
		throw std::exception("back() or push_back() error");

	rb.push_back(val_b);
	if (rb.front() != val_b)
		throw std::exception("front() or push_back() error");

	if (N > 1 && rb.back() == val_b)
		throw std::exception("push_back() error");

	rb.fold([val_a](auto& val) { val = val_a; });
	for (const auto& val : rb)
		if (val != val_a)
			throw std::exception("fold assign error");
}

template<typename T, size_t N> void test_range(T val_a, T val_b)
{
	test_impl<T, N>(val_a, val_b);

	ring_buffer<T, N> rb;
	
	if (N == 1)
	{
		rb.push_back(val_a);
		auto res = merge_ring_range(rb);
		if (res != val_a)
			throw std::exception("merge range size 1 error");
		return;
	}
	else if (N == 2)
	{
		rb.push_back(val_b);
		rb.push_back(val_a);
		auto res = merge_ring_range(rb);
		if (res != val_a + val_b)
			throw std::exception("merge range size 2 error");
		return;
	}

	rb.push_back(val_b);
	for (size_t i = 0; i < N - 2; ++i)
		rb.push_back(val_a);
	rb.push_back(val_b);

	auto manual = val_b;
	for (size_t i = 0; i < N - 2; ++i)
		manual += val_a;
	manual += val_b;

	auto res = merge_ring_range(rb);
	if (res != manual)
		throw std::exception("merge range size > 2 error");
}

void test_RingBuffer()
{
	//test_impl<int, 0>(4, 8);
	test_impl<int, 1>(7, 5);
	test_impl<float, 3>(
		std::numeric_limits<float>::min(),
		std::numeric_limits<float>::max()
	);

	test_range<std::string, 1>("short", "a bit longer, and even more");
	test_range<std::string, 2>("short", "a bit longer, and even more");
	test_range<std::string, 3>("short", "a bit longer, and even more");
	test_range<std::string, 5>("short", "a bit longer, and even more");
	test_range<std::string, 100>("short", "a bit longer, and even more");
}

}
}