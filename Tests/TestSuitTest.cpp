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
		suit_configuration suitConfiguration;
		suitConfiguration.gnuplotPath = "C:\\ProgramData\\gnuplot\\bin";
		suitConfiguration.resultDataPath = "PlotResults\\PlotData\\";
		suitConfiguration.resultName = "TST_";
		suitConfiguration.warningsActive = true;

		size_t size = 100;
		size_t stepCount = 50;
		size_t repCount = 3;

		auto testA = createTest("build vec",
			[](const size_t size)
			{ std::vector<int> v(size); }
		);

		auto resultTestA = testA.runTest(size, 3);

		auto emptyTest = createTest("Empty test", []() {});

		if (resultTestA.first != size)
			throw std::exception("createTest or runTest or getResult error");

		// allocating a vector of size and measuring time should not take 0 nanoseconds
		if (!(resultTestA.second > 0))
			throw std::exception("Full test time was 0 nanoseconds");

		const char* nameA = "Test_category_A";
		auto categoryA = createTestCategory(nameA, testA);

		if (categoryA.name() != nameA)
			throw std::exception("category name set or get error");

		auto emptyCategory = ("Empty");
		static_cast<void>(emptyCategory);

		auto testB = createTest("build vec and sort",
			[](const size_t size)
			{
				std::vector<std::vector<int>> v(size);
				std::sort(v.begin(), v.end());
			}
		);

		{
			auto rangeResult = categoryA.runTestRange(0, size, stepCount, repCount);
			if (rangeResult.back().points().front().first != 0)
				throw std::exception("runTestRange() first test size not 0");

			if (rangeResult.back().points().back().first != size)
				throw std::exception("runTestRange() first test size not size");

			rangeResult = categoryA.runTestRange(0, 0, stepCount, repCount);
			if (rangeResult.back().points().size() != 1)
				throw std::exception("runTestRange() not 1 results in range 0-0");

			rangeResult = categoryA.runTestRange(size, size, stepCount, repCount);
			if (rangeResult.back().points().size() != 1)
				throw std::exception("runTestRange() not 1 results in range size-size");

			rangeResult = categoryA.runTestRange(0, 1, stepCount, repCount);
			if (rangeResult.back().points().size() != 2)
				throw std::exception("runTestRange() not 2 results in range 0-1");
		}

		{
			TestSuit<decltype(categoryA)> suitA(suitConfiguration, categoryA);
			TestSuit<decltype(categoryA)> suitB(suitConfiguration, categoryA);
			//suitA = std::move(suitB);
		}

		auto suitA = createTestSuit(suitConfiguration, categoryA);

		suitA.runAllTests(size, size);

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