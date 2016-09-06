/*
* Vool - Unit tests for Utility
*
* Copyright (C) 2016 by Lukas Bergdoll - www.lukas-bergdoll.net
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#include "AllTests.h"

#include <Utility.h>

#include <vector>
#include <exception>

namespace vool
{

namespace test
{

void test_Utility()
{
	// tmp
	using Ingredient = vool::ArithmeticStruct<float, int, double, char>;
	auto cookie1 = Ingredient(3.4f, 42, 55.66, 'd');
	auto cookie2 = Ingredient(4.f, 42, 72.3, 'z');
	auto result = cookie1 * cookie2 + cookie1 - cookie2;

	//auto emptyTest = vool::ArithmeticStruct<>(); // compiler error
	//emptyTest = emptyTest + emptyTest; // compiler error
	//std::cout << emptyTest.are_all_positive() << std::endl; // compiler error

	std::vector<int> v;
	// compiler error
	//auto wrongTypeTest = vool::ArithmeticStruct<int, float, decltype(v)>(2, 3.f, v);

	// there is no 64-bit int type able to hold all the data, so it is double
	vool::util::needed_arith_type_t<int16_t, uint32_t> impossibleRange = -2;
	std::common_type_t<int16_t, uint32_t> impossibleRangeCT = -2;
	

	//vool::util::needed_arith_type_t<int, std::vector<int>> incompatible; // compiler error

}

}

}