/*
* Vool - Unit tests for test_suit, as it is highly generic, many cases won't be tested
*
* Copyright (c) 2016 Lukas Bergdoll - www.lukas-bergdoll.net
*
* This code is licensed under the AFL-3.0 license (http://opensource.org/licenses/AFL-3.0)
*/

#include "AllTests.h"

#include <TestSuit.h>

#include <vector>
#include <string>
#include <unordered_set>
#include <exception>

namespace vool
{

namespace tests
{

void test_TestSuit()
{
	// test Result, test, test_category and test_suit
	{
		suit_config suit_configuration;
		suit_configuration.gnuplot_path = "C:\\ProgramData\\gnuplot\\bin";
		suit_configuration.output_filepath = "PlotResults\\PlotData\\";
		suit_configuration.filename = "TST_";
		suit_configuration.warnings_active = true;

		size_t size = 100;
		size_t steps = 50;
		size_t repetitions = 3;

		auto testA = make_test("build vec",
			[](const size_t size)
			{ std::vector<int> v(size); }
		);

		auto resultTestA = testA.run_test(size, 3);

		auto emptyTest = make_test("Empty test", []() {});

		if (resultTestA.first != size)
			throw std::exception("make_test or run_test or getResult error");

		// allocating a vector of size and measuring time should not take 0 nanoseconds
		if (!(resultTestA.second > 0))
			throw std::exception("Full test time was 0 nanoseconds");

		const char* nameA = "Test_category_A";
		auto categoryA = make_test_category(nameA, testA);

		if (categoryA.name() != nameA)
			throw std::exception("category name set or get error");

		auto emptyCategory = ("Empty");
		static_cast<void>(emptyCategory);

		auto testB = make_test("build vec and sort",
			[](const size_t size)
			{
				std::vector<std::vector<int>> v(size);
				std::sort(v.begin(), v.end());
			}
		);

		{
			auto rangeResult = categoryA.perform_tests(0, size, steps, repetitions);
			if (rangeResult.back().points().front().first != 0)
				throw std::exception("perform_tests() first test size not 0");

			if (rangeResult.back().points().back().first != size)
				throw std::exception("perform_tests() first test size not size");

			rangeResult = categoryA.perform_tests(0, 0, steps, repetitions);
			if (rangeResult.back().points().size() != 1)
				throw std::exception("perform_tests() not 1 results in range 0-0");

			rangeResult = categoryA.perform_tests(size, size, steps, repetitions);
			if (rangeResult.back().points().size() != 1)
				throw std::exception("perform_tests() not 1 results in range size-size");

			rangeResult = categoryA.perform_tests(0, 1, steps, repetitions);
			if (rangeResult.back().points().size() != 2)
				throw std::exception("perform_tests() not 2 results in range 0-1");
		}

		auto suitA = make_test_suit(suit_configuration, categoryA);

		suitA.perform_categorys(size, size);

		auto categoryB = make_test_category("container_build", testA, testB);
		auto suitB = make_test_suit(suit_configuration, categoryA, categoryB);
		suitB.perform_categorys(0, size);
		suitB.render_results();

		// test empty category
		auto invisibleTest = testB;
		invisibleTest.flag_invisible();
		auto invisibleCategory = make_test_category("invisible Category", invisibleTest);
		auto suitC = make_test_suit(suit_configuration, invisibleCategory, categoryA);
		suitC.perform_categorys(0, size);
		suitC.render_results();
	}

	// test generate_container()
	{
		auto gen_test = [](const auto& config)
		{
			auto vec = generate_container(config);

			using config_t = typename std::decay<decltype(config)>::type::type;

			static_assert(
				std::is_same<
				decltype(vec),
				std::vector<config_t>
				>::value,
				"generate_container() wrong return type!"
				);

			if (vec.size() != config.size)
				throw std::exception("generate_container() returned container with wrong size");

			for (const auto& element : vec)
				if (element < config.lower_bound || element > config.upper_bound)
					throw std::exception("generate_container() invalid boundarys");

			if (config.unique)
			{
				std::unordered_set<config_t> set;
				for (const auto& element : vec)
				{
					if (set.count(element) != 0)
						throw std::exception("generate_container() values not unique");
					set.insert(element);
				}
			}
		};

		// test unique int
		{
			ContainerConfig<int> config;
			config.size = 100;
			config.lower_bound = 0;
			config.upper_bound = 1000;
			config.unique = true;
			gen_test(config);
		}

		// test float
		{
			ContainerConfig<float> config;
			config.size = 100;
			config.lower_bound = -5.f;
			config.upper_bound = 3.f;

			config.unique = false;
			gen_test(config);

			config.unique = true;
			gen_test(config);
		}
	}
}

}

}