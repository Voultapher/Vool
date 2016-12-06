/*
* Vool - Unit tests for fixed_queue
*
* Copyright (c) 2016 Lukas Bergdoll - www.lukas-bergdoll.net
*
* This code is licensed under the Apache License 2.0 (https://opensource.org/licenses/Apache-2.0)
*/

#include "AllTests.h"

#include <FixedQueue.h>

#include <string>
#include <iostream>

namespace vool
{
namespace tests
{

template<typename T, size_t N> void test_impl(T val_a, T val_b)
{
	fixed_queue<T, N> rb;

	size_t count_folds{};
	rb.fold([&count_folds](const auto& val) -> void { ++count_folds; });
	if (count_folds != 0)
		throw std::exception("fold on uninitialized");

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

	rb.fold([val_a](auto val)
		{
			if (val != val_a)
				throw std::exception("fold assign error");
		}
	);
}

template<typename T, size_t N> void test_range(T val_a, T val_b)
{
	test_impl<T, N>(val_a, val_b);

	fixed_queue<T, N> rb;
	
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

void test_FixedQueue()
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

	//fixed_queue<std::string, 5> rb;
	//auto res = merge_ring_range(rb);
	//std::cout << "merge: " << res << '\n';

	//rb.push_back("1");
	//res = merge_ring_range(rb);
	//std::cout << "merge: " << res << '\n';

	//rb.push_back("2");
	//res = merge_ring_range(rb);
	//std::cout << "merge: " << res << '\n';

	//rb.push_back("3");
	//res = merge_ring_range(rb);
	//std::cout << "merge: " << res << '\n';

	//rb.push_back("4");
	//res = merge_ring_range(rb);
	//std::cout << "merge: " << res << '\n';

	//rb.push_back("5");
	//res = merge_ring_range(rb);
	//std::cout << "merge: " << res << '\n';

	//rb.push_back("6");
	//res = merge_ring_range(rb);
	//std::cout << "merge: " << res << '\n';

	//fixed_queue<int, 5> uninit_int_rb;
	//uninit_int_rb.fold([](const auto& val) { std::cout << val << '\n'; });
}

}
}