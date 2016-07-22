/*
* Vool - Unit tests for TestSuit, as it is highly generic, many cases wont be tested
*
* Copyright (C) 2016 by Lukas Bergdoll - www.lukas-bergdoll.net
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/
#pragma once

#include <vector>
#include <string>
#include <stdexcept>

#include <TestSuit.h>

namespace vool
{
const char* test_TestSuit()
{
	Result resultA(2, 100.0, 60, "Simple Result");
	if (resultA.getSize() != 2)
		throw std::exception(); // Result construction or getSize() failed

	size_t size = 100;
	auto testA = createTest("Build vec", [](const size_t size) {std::vector<int> v(size); });
	testA.runTest(size);
	auto resultTestA = testA.getResult();

	auto emptyTest = createTest("Empty test", []() {});

	if (resultTestA.getSize() != size)
		throw std::exception(); // createTest/ runTest/ getResult error

	// allocating a vector of size and measuring time should not take 0 nanoseconds
	if (!(resultTestA.getFullTime() > 0))
		throw std::exception();

	auto categoryA = createTestCategory("Test_category_A", testA);
	categoryA.runTestRange(0, size, 50);
	auto resultCategoryA = categoryA.getResults();

	if (resultCategoryA.second != "Test_category_A")
		throw std::exception(); // category name set or get error
	if (resultCategoryA.first.back().back().getSize() > size)
		throw std::exception(); // category range test error

	SuitConfiguration suitConfiguration;
	suitConfiguration.gnuplotPath = "C:\\ProgramData\\gnuplot\\bin";
	suitConfiguration.resultDataPath = "PlotResults\\PlotData\\";
	suitConfiguration.resultName = "TST_";

	auto emptyCategory = ("Empty");

	auto suitA = createTestSuit(suitConfiguration, categoryA);
	suitA.runAllTests(0, size);
	auto resultSuitA = suitA.getResults();
	if (resultSuitA.front().first.back().back().getSize() > size)
		throw std::exception(); // runAllTests or getResults error

	suitA.runAllTests(0, 0);
	suitA.runAllTests(0, 1);
	resultSuitA = suitA.getResults();
	if (resultSuitA.front().first.back().back().getSize() != 1)
		throw std::exception(); // runAllTests error, range fault

	suitA.runAllTests(size, size);
	resultSuitA = suitA.getResults();
	if (resultSuitA.front().first.back().back().getSize() != size)
		throw std::exception(); // runAllTests error, range fault
	if (resultSuitA.front().first.back().size() != 1) // there should only be one result
		throw std::exception(); // runAllTests error, range fault

	auto testB = createTest("Build 2D vector", [](const size_t size)
	{ std::vector<std::vector<int>> v(size); });
	auto categoryB = createTestCategory("container_build", testA, testB);
	auto suitB = createTestSuit(suitConfiguration, categoryA, categoryB);
	suitB.runAllTests(0, size);
	suitB.renderResults();

	// test empty category
	auto invisibleTest = testB;
	invisibleTest.setInvisible();
	auto invisibleCategory = createTestCategory("invisible Category", invisibleTest);
	auto suitC = createTestSuit(suitConfiguration, invisibleCategory, categoryA);
	suitC.runAllTests(0, size);
	suitC.renderResults();

	return "Testsuit test was successful!\n";
}

}