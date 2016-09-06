/*
* Vool - Unit tests for TestSuit, as it is highly generic, many cases wont be tested
*
* Copyright (C) 2016 by Lukas Bergdoll - www.lukas-bergdoll.net
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#include "AllTests.h"

#include <TestSuit.h>

#include <vector>
#include <string>
#include <unordered_set>
#include <exception>

namespace vool
{

namespace test
{

void test_TestSuit()
{
	// test Result, Test, TestCategory and TestSuit
	{
		Result resultA(2, 100, 60, "Simple Result");
		if (resultA.getSize() != 2)
			throw std::exception("Result construction or getSize() failed");

		size_t size = 100;
		auto testA = createTest("Build vec", [](const size_t size) {std::vector<int> v(size); });
		testA.runTest(size, 3);
		auto resultTestA = testA.getResult();

		auto emptyTest = createTest("Empty test", []() {});

		if (resultTestA.getSize() != size)
			throw std::exception("createTest or runTest or getResult error");

		// allocating a vector of size and measuring time should not take 0 nanoseconds
		if (!(resultTestA.getFullTime() > 0))
			throw std::exception("Full test time was 0 nanoseconds");

		auto categoryA = createTestCategory("Test_category_A", testA);
		categoryA.runTestRange(0, size, 50, 3);
		auto resultCategoryA = categoryA.getResults();

		if (resultCategoryA.second != "Test_category_A")
			throw std::exception("category name set or get error");
		if (resultCategoryA.first.back().back().getSize() > size)
			throw std::exception("category range test error");

		SuitConfiguration suitConfiguration;
		suitConfiguration.warningsActive = true;
		suitConfiguration.gnuplotPath = "C:\\ProgramData\\gnuplot\\bin";
		suitConfiguration.resultDataPath = "PlotResults\\PlotData\\";
		suitConfiguration.resultName = "TST_";

		auto emptyCategory = ("Empty");
		static_cast<void>(emptyCategory);

		auto testB = createTest("Build 2D vector", [](const size_t size)
		{ std::vector<std::vector<int>> v(size); });

		{
			TestSuit<decltype(categoryA)> suitA(suitConfiguration, categoryA);
			TestSuit<decltype(categoryA)> suitB(suitConfiguration, categoryA);
			//suitA = std::move(suitB);
		}

		auto suitA = createTestSuit(suitConfiguration, categoryA);

		suitA.runAllTests(0, size);
		auto resultSuitA = suitA.getResults();
		if (resultSuitA.front().first.back().back().getSize() > size)
			throw std::exception("runAllTests or getResults error");

		suitA.runAllTests(0, 0);
		suitA.runAllTests(0, 1);
		resultSuitA = suitA.getResults();
		if (resultSuitA.front().first.back().back().getSize() != 1)
			throw std::exception("runAllTests error, range fault");

		suitA.runAllTests(size, size);
		resultSuitA = suitA.getResults();
		if (resultSuitA.front().first.back().back().getSize() != size)
			throw std::exception("runAllTests error, range fault");
		if (resultSuitA.front().first.back().size() != 1) // there should only be one result
			throw std::exception("runAllTests error, range fault");

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
	}

	// test generateContainer()
	{
		auto gen_test = [](const auto& config)
		{
			auto vec = generateContainer(config);

			using config_t = typename std::decay<decltype(config)>::type::type;

			static_assert(
				std::is_same<
				decltype(vec),
				std::vector<config_t>
				>::value,
				"generateContainer() wrong return type!"
				);

			if (vec.size() != config.size)
				throw std::exception("generateContainer() returned container with wrong size");

			for (const auto& element : vec)
				if (element < config.lowerBound || element > config.upperBound)
					throw std::exception("generateContainer() invalid boundarys");

			if (config.unique)
			{
				std::unordered_set<config_t> set;
				for (const auto& element : vec)
				{
					if (set.count(element) != 0)
						throw std::exception("generateContainer() values not unique");
					set.insert(element);
				}
			}
		};

		// test unique int
		{
			ContainerConfig<int> config;
			config.size = 100;
			config.lowerBound = 0;
			config.upperBound = 1000;
			config.unique = true;
			gen_test(config);
		}

		// test float
		{
			ContainerConfig<float> config;
			config.size = 100;
			config.lowerBound = -5.f;
			config.upperBound = 3.f;

			config.unique = false;
			gen_test(config);

			config.unique = true;
			gen_test(config);
		}
	}
}

}

}