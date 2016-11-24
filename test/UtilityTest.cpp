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
		"is_detected expected to indicate range.begin()!");

	static_assert(Expec || !def_begin,
		"is_detected expected to indicate NO T.begin()!");

	// #B
	constexpr bool type_val = util::is_detected_v<util::type_val, T>;
	static_assert(!Expec || type_val,
		"is_detected expected to indicate range::value_type!");

	static_assert(Expec || !type_val,
		"is_detected expected to indicate NO T::value_type!");

	// #C
	constexpr bool range_begin_it = util::has_range_begin<T>::value;
	static_assert(!Expec || range_begin_it,
		"has_range_begin expected to indicate a range.begin()!");

	static_assert(Expec || !range_begin_it,
		"has_range_begin expected to indicate NO T.begin()!");

	// #D
	constexpr bool range_end_it = util::has_range_end<T>::value;
	static_assert(!Expec || range_end_it,
		"has_range_end expected to indicate a range.end()!");

	static_assert(Expec || !range_end_it,
		"has_range_end expected to indicate NO T.end()!");

	// #E
	constexpr bool range_iterator = util::has_range_iterator<T>::value;
	static_assert(!Expec || range_iterator,
		"has_range_iterator expected to indicate a range_iterator!");

	static_assert(Expec || !range_iterator,
		"has_range_iterator expected to indicate NO range_iterator!");

	// #F
	constexpr bool is_range = util::is_range_v<T>;
	static_assert(!Expec || is_range,
		"is_range expected to indicate a range!");

	static_assert(Expec || !is_range,
		"is_range expected to indicate T to NOT be a range!");
}

void general_tests()
{
	using comm_t = util::detected_or_t<
		std::false_type,
		std::common_type_t, int, char, double
	>;
	static_assert(std::is_same<comm_t, double>::value,
		"detected_or_t failed to indicate common_type!");

	using map_fail_t = util::detected_or_t<
		std::false_type,
		util::type_iterator, int
	>;
	static_assert(std::is_same<map_fail_t, std::false_type>::value,
		"detected_or_t failed to indicate NO type int::iterator!");

	/*static_assert(util::is_hashable_v<int>,
	"int should be hashable!");
	static_assert(!util::is_hashable_v<no_cm>,
	"no_cm should not be hashable!");*/

	static_assert(util::is_vector_v<std::vector<int>>,
		"is_vector_v<vec_t> should be true!");
	static_assert(!util::is_vector_v<no_cm>,
		"is_vector_v<no_cm> should be false!");
}

template<typename T> void concepts_test()
{
	general_tests();

	using vec_t = std::vector<T>;
	using vec_vec_t = std::vector<std::vector<T>>;
	using set_t = std::set<T>;
	/*using map_t = util::detected_or_t<
		std::map<T, T>,
		std::unordered_map,
		T, T // this needs Ts... over T in is_detect
	>;*/
	using map_t = std::map<T, T>;

	range_tests<vec_t, true>();
	range_tests<vec_vec_t, true>();
	range_tests<set_t, true>();
	range_tests<map_t, true>();

	range_tests<int, false>();
	range_tests<double, false>();
	range_tests<char, false>();
	range_tests<no_cm, false>();
	range_tests<void, false>();
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
	concepts_test<no_cm>();
}

}

}