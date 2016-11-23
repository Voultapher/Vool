/*
* Vool - Unit tests for Utility
*
* Copyright (c) 2016 Lukas Bergdoll - www.lukas-bergdoll.net
*
* This code is licensed under the Apache License 2.0 (https://opensource.org/licenses/Apache-2.0)
*/

#include "AllTests.h"

#include <Utility.h>

#include <vector>
#include <set>
#include <map>
#include <unordered_map>
#include <deque>

namespace vool
{

namespace tests
{

struct no_cm
{
	int val;
	explicit no_cm(const int v) : val(v) {};
	~no_cm() {};

	no_cm(const no_cm&) = delete;
	no_cm(no_cm&&) = delete;

	no_cm& operator= (const no_cm&) = delete;
	no_cm& operator= (no_cm&&) = delete;
};

template<typename T, bool Expec> void range_tests()
{
	// #A
	constexpr bool def_begin = util::is_detected_v<util::def_begin, T>;
	static_assert(!Expec || def_begin,
		"is_detected expected to indicate T.begin()!");

	static_assert(Expec || !def_begin,
		"is_detected expected to indicate no T.begin()!");

	// #B
	constexpr bool type_val = util::is_detected_v<util::type_val, T>;
	static_assert(!Expec || type_val,
		"is_detected expected to indicate T::value_type!");

	static_assert(Expec || !type_val,
		"is_detected expected to indicate no T::value_type!");

	// #C
	constexpr bool return_begin_it = vool::util::is_detected_expression_v<
		util::type_iterator,
		util::def_begin,
		T
	>;
	static_assert(!Expec || return_begin_it,
		"is_detected_expression_v expected to indicate a T.begin()!");

	static_assert(Expec || !return_begin_it,
		"is_detected_expression_v expected to indicate no T.begin()!");

	// #C
	constexpr bool is_range = util::is_range_v<T>;
	static_assert(!Expec || is_range,
		"is_range expected to indicate a range!");

	static_assert(Expec || !is_range,
		"is_range expected to indicate no range!");
}

template<typename T> void concepts_test()
{
	static_assert(util::is_hashable_v<int>,
		"int should be hashable!");
	static_assert(!util::is_hashable_v<no_cm>,
		"no_cm should not be hashable!");

	using vec_t = std::vector<T>;
	/*using vec_vec_t = std::vector<std::vector<T>>;
	using set_t = std::set<T>;
	using map_t = std::conditional<util::is_hashable_v<T>,
		std::unordered_map<T, T>,
		std::map<T, T>
	>::type;

	range_tests<vec_t, true>();
	range_tests<vec_vec_t, true>();
	range_tests<set_t, true>();
	range_tests<map_t, true>();

	range_tests<int, false>();
	range_tests<double, false>();
	range_tests<char, false>();
	range_tests<no_cm, false>();
	range_tests<void, false>();*/
}

void test_Utility()
{
	// --- variadic compile time property check ---


	// --- variadic logical binary metafunctions ---


	// --- common airthmetic type ---
	static_assert(std::is_same<
		util::common_arithmetic_t<int16_t, uint32_t>,
		int64_t
	>::value, "common_arithmetic_t invalid range!");

	// --- tuple iteration ---



	// ----- Concepts -----
	concepts_test<int>();
	//concepts_test<no_cm>();
}

}

}