/*
* Vool - Example usage of Vool
*
* Copyright (C) 2016 by Lukas Bergdoll - www.lukas-bergdoll.net
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#include <cstdio>
#include <iostream>
#include <vector>

#include "Tests\VecmapTest.h"
#include "Tests\GNPTest.h"
#include "Tests\TestSuitTest.h"
#include "Tests\TaskQueueTest.h"

#include "Vool.h"

void runUnitTest(const char* name, std::function<void()> func)
{
	try
	{
		func();
		std::cout << "passed: " << name << "\n";
	}
	catch (std::exception& e)
	{
		std::cout << "failed: " << name << " - " << e.what() << "\n";
	}
}

int main()
{
	std::cout << "\tRunning unit tests:\n\n";

	runUnitTest("Vecmap", vool::test_Vecmap);
	runUnitTest("GNP", vool::test_GNP);
	runUnitTest("TestSuit", vool::test_TestSuit);
	runUnitTest("TaskQueue", vool::test_TaskQueue);

	std::cout << "\n\tAll unit test done!\n\n" << std::flush;

	char userInput = std::cin.get();
	if (userInput == '\n')
		return 0;

	using Ingredient = vool::ArithmeticStruct<float, int, double, char>;
	auto cookie1 = Ingredient(3.4f, 42, 55.66, 'd');
	auto cookie2 = Ingredient(4.f, 42, 72.3, 'z');
	auto result = cookie1 * cookie2 + cookie1 - cookie2;
	std::cout
		<< "Result sum is: "
		<< result.sum()
		<< " and the first item is: "
		<< result.front() << std::endl;

	std::cout << "\nPrinting all elements of result: \n";

	// prints all elements, although may out of order
	result.doForAll([](const auto& element)
		{std::cout << element << std::endl; });

	//auto emptyTest = vool::ArithmeticStruct<>(); // compiler error
	//emptyTest = emptyTest + emptyTest; // compiler error
	//std::cout << emptyTest.are_all_positive() << std::endl; // compiler error

	std::vector<int> v;
	// compiler error
	//auto wrongTypeTest = vool::ArithmeticStruct<int, float, decltype(v)>(2, 3.f, v);

	// there is no 64-bit int type able to hold all the data, so it is double
	vool::util::needed_arith_type_t<int16_t, uint32_t> impossibleRange = -2;
	std::common_type_t<int16_t, uint32_t> impossibleRangeCT = -2;
	std::cout
		<< "\nFinding valid arithmetic type, with example types:"
		<< "\n\n\t<int16_t, uint32_t> and -2 as value\n"
		<< "\nVariable content using common_type: "
		<< impossibleRangeCT
		<< "\nVariable content using vool::util::needed_arith_type: "
		<< impossibleRange
		<< "\n";

	//vool::util::needed_arith_type_t<int, std::vector<int>> incompatible; // compiler error

	std::cout << "\nDone!\n" << std::flush;

	std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	std::cin.get();

	return 0;
}