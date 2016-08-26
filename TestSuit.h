/*
* Vool - Generic perfomance test suit using GNP to visualize results
*
* Copyright (C) 2016 by Lukas Bergdoll - www.lukas-bergdoll.net
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#pragma once

#include <iostream>
#include <vector>
#include <algorithm>
#include <functional>
#include <iterator>
#include <unordered_set>
#include <string>
#include <tuple>

#include <chrono>
#include <random>

#include "GNP.h"

namespace vool
{

struct Result
{
private:
	size_t _size;
	int64_t _fullTime;
	int64_t _averageTime;
	std::string _taskName;

public:
	Result(const size_t size, const int64_t fT, const int64_t aT, const std::string& tN) :
		_size(size), _fullTime(fT), _averageTime(aT), _taskName(tN) {}

	const size_t getSize() const { return _size; }
	const int64_t getFullTime() const { return _fullTime; }
	const int64_t getAverageTime() const { return _averageTime; }
	const std::string& getTaskName() const { return _taskName; }
};

template<typename TestFunc> class Test
{
private:

	//Container_t _container;
	bool _visible;
	TestFunc _testFunc;
	Result _result;

	std::string _testName;

	inline const auto timerStart() const
	{
		return std::chrono::high_resolution_clock::now();
	}

	inline void timerEnd(
		const std::chrono::high_resolution_clock::time_point& start,
		const size_t iterations
	)
	{
		auto end = std::chrono::high_resolution_clock::now(); // save end time

		int64_t deltaTimeNano = std::chrono::duration_cast<std::chrono::nanoseconds>
			(end - start).count(); // calculate difference in nanosecons
		int64_t deltaTimeSec = deltaTimeNano / static_cast<int64_t>(1e9);

		// calculate how long each iteration took on average
		int64_t averageIndividualTime = deltaTimeNano / iterations;

		_result = std::move(Result(iterations, deltaTimeNano, averageIndividualTime, _testName));
	}

public:

	explicit Test(TestFunc& func, const std::string& testName)
		: _visible(true),
		_testFunc(func),
		_testName(testName),
		_result(0, 0, 0, "No Test") { }

	void setInvisible() { _visible = false; }

	void runTest(const size_t size)
	{
		auto start = timerStart();
		_testFunc(size);
		timerEnd(start, size);
	}

	const bool isVisible() const { return _visible; }
	const Result& getResult() const { return _result; }
};

template<typename Func>
Test<Func> createTest(const std::string& testName, Func func)
{
	return Test<Func>(std::ref(func), testName);
}

template<typename... Tests> class TestCategory
{
private:
	std::tuple<Tests...> _tests;
	std::vector<std::vector<Result>> _results;
	std::string _categoryName;

	// iteration, only tuple as argument
	template<typename F, typename... Ts, std::size_t... Is>
	void for_each_in_tuple(std::tuple<Ts...>& tuple, F func, std::index_sequence<Is...>)
	{
		// execution order matters
		static_cast<void>
			(std::initializer_list<int> { (func(std::get<Is>(tuple)), 0)... });
	}

	template<typename F, typename...Ts>
	void for_each_in_tuple(std::tuple<Ts...>& tuple, F func)
	{
		// call for_each_in_tuple with the constructed index_sequence
		for_each_in_tuple(tuple, func, std::make_index_sequence<sizeof...(Ts)>());
	}

public:
	explicit TestCategory(const std::string& categoryName, Tests&... tests)
		: _categoryName(categoryName), _tests(std::make_tuple(tests...)) { }

	void runTestRange(const size_t minSize, const size_t maxSize, const size_t stepCount)
	{ // all test will be executed in construction order

		// size construct to fit the amount of tests
		std::vector<std::vector<Result>> categoryResults(sizeof...(Tests));

		auto runCategoryTests = [this, &categoryResults](const size_t size)
		{
			auto categoryResults_it = categoryResults.begin();

			for_each_in_tuple(_tests,
				[&categoryResults_it, size, &categoryResults](auto& element)
				{
					element.runTest(size);

					if (element.isVisible())
					{
						categoryResults_it->push_back(element.getResult());
						++categoryResults_it;
					}
				}
			);
		};

		// runCategoryTests() should only be called if:
		// maxsize is larger than 0 and larger or equal to minsize
		if (minSize <= 1)
			runCategoryTests(1);

		size_t size = 2;
		for (; size < maxSize && size >= minSize; size += 1 + maxSize / stepCount)
			runCategoryTests(size);

		if (maxSize > 1)
			runCategoryTests(maxSize);

		categoryResults.shrink_to_fit();

		_results = std::move(categoryResults);
	}

	std::pair<decltype(_results), std::string> getResults()
	{
		return std::make_pair(_results, _categoryName);
	};
};

template<typename... Tests> TestCategory<Tests...>
createTestCategory (const std::string& categoryName, Tests&... tests)
{
	return TestCategory<Tests...>(categoryName, tests...);
}

struct SuitConfiguration
{
	uint32_t xResolution;
	uint32_t yResolution;
	bool warningsActive;
	bool pngOutput;
	size_t stepCount;
	std::string gnuplotPath;
	std::string xAxisName;
	std::string yAxisName;
	std::string resultDataPath;
	std::string resultName;

	explicit SuitConfiguration()
		: xResolution(1000), yResolution(500),
		warningsActive(true),
		pngOutput(true),
		stepCount(20),
		gnuplotPath("C:\\ProgramData\\gnuplot\\bin"),
		xAxisName("Size"), yAxisName("Full Time in nanoseconds"),
		resultDataPath(""), resultName("Result") { }
};

// TestSuit
template<typename... TestCategorys> class TestSuit
{
private:
	std::tuple<TestCategorys...> _categorys;
	std::vector<std::pair<std::vector<std::vector<Result>>, std::string>> _results; // oy

	SuitConfiguration _suitConfiguration;

	Gnuplot _plot;

	template<typename... Ts> void wrapper(Ts&&... args) { }

	// iteration, only tuple as argument
	template<typename F, typename... Ts, std::size_t... Is>
	void for_each_in_tuple(std::tuple<Ts...>& tuple, F func, std::index_sequence<Is...>)
	{
		// execution order matters
		static_cast<void>
			(std::initializer_list<int> { (func(std::get<Is>(tuple)), 0)... });
	}

	template<typename F, typename...Ts>
	void for_each_in_tuple(std::tuple<Ts...>& tuple, F func)
	{
		// call for_each_in_tuple with the constructed index_sequence
		for_each_in_tuple(tuple, func, std::make_index_sequence<sizeof...(Ts)>());
	}

public:

	// constructor
	explicit TestSuit(
		const SuitConfiguration& suitConfiguration,
		TestCategorys&... categorys
	) :
		_suitConfiguration(suitConfiguration),
		_categorys(std::make_tuple(categorys...)),
		_plot(_suitConfiguration.gnuplotPath, false) // may throw
	{ }

	// move constructor
	TestSuit(TestSuit<TestCategorys...>&& other) = default;

	// move operator
	TestSuit& operator=(TestSuit<TestCategorys...>&& other) = default;

	// no copy constructor
	TestSuit(const TestSuit& other) = delete;

	// no copy operator
	TestSuit& operator=(const TestSuit& other) = delete;


	void runAllTests(const size_t minSize, const size_t maxSize)
	{ // all test will be executed in construction order
		if (maxSize > 0 && maxSize >= minSize)
		{
			_results.clear();
			auto categoryResults = std::move(_results);

			// for every category
			for_each_in_tuple(_categorys, [minSize, maxSize, &categoryResults,
				stepCount = _suitConfiguration.stepCount](auto& element)
			{
				element.runTestRange(minSize, maxSize, stepCount);
				categoryResults.push_back(element.getResults());
			});
			_results = std::move(categoryResults);
		}
	}

	void renderResults()
	{
		_plot.setWindowMode(_suitConfiguration.xResolution, _suitConfiguration.yResolution);

		_plot << "set samples 500";
		_plot.addLineStyle(1, "#FF5A62", 2, 3, 5, 1.5f);
		_plot.addLineStyle(2, "#2E9ACC", 2, 3, 6, 1.5f);
		_plot.addLineStyle(3, "#9871FF", 2, 3, 7, 1.5f);
		_plot.addLineStyle(4, "#E8803A", 2, 3, 8, 1.5f);
		_plot.addLineStyle(5, "#46E86C", 2, 3, 9, 1.5f);
		_plot.addGrid();
		_plot.setAxis(_suitConfiguration.xAxisName, _suitConfiguration.yAxisName);

		for (const auto& category : _results)
		{
			if (category.first.size() > 0)
			{
				std::vector<PlotData2D<double>> data;
				data.reserve(category.first.size());

				for (const auto& results : category.first)
				{
					if (results.size() > 0)
					{
						std::vector<std::pair<double, double>> points;
						points.reserve(results.size());
						for (const auto& result : results)
							points.push_back
							(
								std::make_pair(
								result.getSize(),
								result.getFullTime())
							);

						data.emplace_back
						(
							std::move(points),
							results.back().getTaskName()
						);
					}
				}

				// sort test results top to bottom
				std::sort(data.begin(), data.end(),
					[](const auto& pointsA, const auto& pointsB)
				{
					return pointsA.getData().back() > pointsB.getData().back();
				});

				for (size_t i = 0; i < data.size(); ++i)
					data[i].setIndexAndLineStyle(i, i + 1);

				if (data.size() > 0)
				{
					// if there are results, write them to a .dat file
					_plot.writeAndPlotData(
						data,
						_suitConfiguration.resultDataPath + category.second + ".dat");

					if (_suitConfiguration.pngOutput)
					{
						// tell gnu_plot to create a .png
						_plot.setSaveMode(
							_suitConfiguration.xResolution,
							_suitConfiguration.yResolution);
						_plot.setOutput(_suitConfiguration.resultName + category.second);
						_plot.plotData(
							data,
							_suitConfiguration.resultDataPath + category.second + ".dat");
					}
				}
				else
					if (_suitConfiguration.warningsActive)
						std::cout
						<< "The category: \""
						<< category.second
						<< "\" had 0 results!\n";
			}
		}
	}

	const decltype(_results)& getResults() const { return _results; }
};

template <typename... Ts> TestSuit<Ts...>
createTestSuit(const SuitConfiguration& suitConfiguration, Ts&... categorys)
{
	return TestSuit<Ts...>(suitConfiguration, categorys...);
}

template<typename T> struct ContainerConfig
{
	using type = T;

	size_t size; // resulting size of the container
	T lowerBound; // lowest possible value in container
	T upperBound; // highest possible value in container
	bool unique; // true should make all values unique

	ContainerConfig() :
		size(0),
		lowerBound(std::numeric_limits<T>::min()),
		upperBound(std::numeric_limits<T>::max()),
		unique(true)
	{
		static_assert(std::is_arithmetic<T>::value, "Type T has to be arithmetic!");
	}
};

template<typename T> std::vector<T>
generateContainer(ContainerConfig<T> config)
{
	static_assert(std::is_arithmetic<T>::value, "Type T has to be arithmetic!");

	std::vector<T> ret(config.size);

	using distribution_t = std::conditional_t<
		std::is_floating_point<T>::value,
		std::uniform_real_distribution<T>,
		std::uniform_int_distribution<T>
	>;

	std::mt19937_64 generator(1580); // fixed seed for reproducibility

	distribution_t distribution(config.lowerBound, config.upperBound);

	if (config.unique)
	{
		if (std::is_integral<T>::value &&
			config.upperBound - config.lowerBound < config.size)
		{
			throw std::exception("container cannot be unique, given config boundarys");
		}

		std::unordered_set<T> set;
		for (auto& element : ret)
		{
			do
			{
				element = distribution(generator);
			} while (set.count(element) != 0);
			set.insert(element);
		}
	}
	else
		for (auto& element : ret)
			element = distribution(generator);

	return std::move(ret);
}

}