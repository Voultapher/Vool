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
	double _fullTime;
	double _averageTime;
	std::string _taskName;

public:
	Result(const size_t size, const double fT, const double aT, const std::string& tN) :
		_size(size), _fullTime(fT), _averageTime(aT), _taskName(tN) {}

	const size_t getSize() const { return _size; }
	const double getFullTime() const { return _fullTime; }
	const double getAverageTime() const { return _averageTime; }
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

	inline void timerEnd(const std::chrono::high_resolution_clock::time_point& start, const size_t iterations)
	{
		std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now(); // save end time

		double deltaTimeNano = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count(); // calculate difference in nanosecons
		double deltaTimeSec = deltaTimeNano / 1e9;
		double averageIndividualTime = deltaTimeNano / iterations; // calculate how long each iteration took on average

		_result = std::move(Result(iterations, deltaTimeNano, averageIndividualTime, _testName));
	}

public:

	explicit Test(TestFunc& func, const std::string& testName)
		: _visible(true), _testFunc(func), _testName(testName), _result(0, 0, 0, "No Test") { }

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

template<typename Func> static auto createTest(const std::string& testName, Func func)
{
	return std::move(Test<Func>(std::ref(func), testName));
}

template<typename... Tests> struct TestCategory
{
private:
	std::tuple<Tests...> _tests;
	std::vector<std::vector<Result>> _results;
	std::string _categoryName;

	// simple iteration, only tuple as argument
	template<class F, class... Ts, std::size_t... Is>
	void for_each_in_tuple(std::tuple<Ts...>& tuple, F func, std::index_sequence<Is...>) {
		(void)std::initializer_list<int> { (func(std::get<Is>(tuple)), 0)... }; // execution order matters
	}

	template<class F, class...Ts>
	void for_each_in_tuple(std::tuple<Ts...>& tuple, F func) {
		for_each_in_tuple(tuple, func, std::make_index_sequence<sizeof...(Ts)>());
	}

public:
	explicit TestCategory(const std::string& categoryName, Tests&... tests)
		: _categoryName(categoryName), _tests(std::make_tuple(tests...)) { }

	void runTestRange(const size_t minSize, const size_t maxSize, const size_t stepCount)
	{ // all test will be executed in construction order
		std::vector<std::vector<Result>> local(sizeof...(Tests)); // resize to fit amount of tests

		auto runTest = [this, &local](const size_t size)
		{
			size_t i = 0;
			for_each_in_tuple(_tests, [&i, size, &local](auto& element)
			{
				element.runTest(size);
				if (element.isVisible())
				{
					local[i].push_back(element.getResult());
					++i;
				}
			});
		};

		// this code should only be called if:
		// maxsize is larger than 0 and larger or equal to minsize
		if (minSize <= 1)
			runTest(1);

		size_t size = 2;
		for (; size < maxSize && size >= minSize; size += 1 + maxSize / stepCount)
			runTest(size);

		if (maxSize > 1)
			runTest(maxSize);

		_results = std::move(local);
	}

	std::pair<decltype(_results), std::string> getResults() { return std::make_pair(_results, _categoryName); };
};

template<typename... Tests> static auto createTestCategory(const std::string& categoryName, Tests&... tests)
{
	return std::move(TestCategory<Tests...>(categoryName, tests...));
}

struct SuitConfiguration
{
	uint32_t xResolution;
	uint32_t yResolution;
	size_t stepCount;
	std::string gnuplotPath;
	std::string xAxisName;
	std::string yAxisName;
	std::string resultDataPath;
	std::string resultName;

	explicit SuitConfiguration(uint32_t xRes = 1200, uint32_t yRes = 500, size_t stepNumber = 20,
		const std::string& gpPath = "C:\\ProgramData\\gnuplot\\bin",
		const std::string& xName = "Size", const std::string& yName = "Full Time in nanoseconds",
		const std::string& resultPath = "", const std::string& resName = "Result") // empty resultPath as a folder would have to be constructed
		: xResolution(xRes), yResolution(yRes), stepCount(stepNumber),
		gnuplotPath(gpPath),
		xAxisName(xName), yAxisName(yName),
		resultDataPath(resultPath), resultName(resName) { }
};

// TestSuit
template<typename... TestCategorys> class TestSuit
{
private:
	std::tuple<TestCategorys...> _categorys;
	std::vector<std::pair<std::vector<std::vector<Result>>, std::string>> _results; // oy

	SuitConfiguration _suitConfiguration;

	template<typename... Ts> void wrapper(Ts&&... args) { }

	// simple iteration, only tuple as argument
	template<class F, class... Ts, std::size_t... Is>
	void for_each_in_tuple(std::tuple<Ts...>& tuple, F func, std::index_sequence<Is...>) {
		(void)std::initializer_list<int> { (func(std::get<Is>(tuple)), 0)... }; // execution order matters
	}

	template<class F, class...Ts>
	void for_each_in_tuple(std::tuple<Ts...>& tuple, F func) {
		for_each_in_tuple(tuple, func, std::make_index_sequence<sizeof...(Ts)>());
	}

public:
	explicit TestSuit(const SuitConfiguration& suitConfiguration, TestCategorys&... categorys)
		: _suitConfiguration(suitConfiguration), _categorys(std::make_tuple(categorys...)) { }

	void runAllTests(const size_t minSize, const size_t maxSize)
	{ // all test will be executed in construction order
		if (maxSize > 0 && maxSize >= minSize)
		{
			_results.clear();
			auto local = std::move(_results);

			// for every category
			for_each_in_tuple(_categorys, [minSize, maxSize, &local,
				stepCount = _suitConfiguration.stepCount](auto& element)
			{
				element.runTestRange(minSize, maxSize, stepCount);
				local.push_back(element.getResults());
			});
			_results = std::move(local);
		}
	}

	void renderResults()
	{
		Gnuplot plot(_suitConfiguration.gnuplotPath, false);
		plot.setSaveMode(_suitConfiguration.xResolution, _suitConfiguration.yResolution);
		plot << "set samples 500";
		plot.addLineStyle(1, "#FF5A62", 2, 3, 5, 1.5f);
		plot.addLineStyle(2, "#2E9ACC", 2, 3, 6, 1.5f);
		plot.addLineStyle(3, "#9871FF", 2, 3, 7, 1.5f);
		plot.addLineStyle(4, "#E8803A", 2, 3, 8, 1.5f);
		plot.addLineStyle(5, "#46E86C", 2, 3, 9, 1.5f);
		plot.addGrid();
		plot.setAxis(_suitConfiguration.xAxisName, _suitConfiguration.yAxisName);

		for (const auto& category : _results)
		{
			if (category.first.size() > 0)
			{
				std::vector<PlotData2D<double>> data;
				data.reserve(category.first.size());

				size_t index = 0;
				for (const auto& results : category.first)
				{
					if (results.size() > 0)
					{
						std::vector<std::pair<double, double>> points;
						points.reserve(results.size());
						for (const auto& result : results)
							points.push_back(std::make_pair(result.getSize(), result.getFullTime()));
						data.emplace_back(std::move(points), index + 1, index, results.back().getTaskName());
						++index;
					}
				}
				if (data.size() > 0)
				{ // if there are results, write them to a .dat file and tell gnuplot to create a .png
					plot.setOutput(_suitConfiguration.resultName + category.second);
					plot.plotData(data, _suitConfiguration.resultDataPath + category.second + ".dat");
				}
				else
					std::cout << "The category: \"" << category.second << "\" had 0 results!\n";
			}
		}
	}

	const decltype(_results)& getResults() const { return _results; }
};

template<typename... TestCategorys> static auto createTestSuit(const SuitConfiguration& suitConfiguration, TestCategorys&... categorys)
{
	return std::move(TestSuit<TestCategorys...>(suitConfiguration, categorys...));
}

template<typename K, typename Func> static auto createUniqueKeys(const size_t size, K startKey, Func keyFunc)
{
	std::vector<K> keyVals;

	std::mt19937_64 generator(1337); // fixed seed for reproducibility
	using rand_t = std::conditional_t < std::is_arithmetic<K>::value && !std::is_floating_point<K>::value, K, int64_t>;
	std::uniform_int_distribution<int64_t> distribution(std::numeric_limits<rand_t>::min(), std::numeric_limits<rand_t>::max());
	std::vector<rand_t> usedRandValues;
	rand_t randVal;

	for (size_t i = 0; i < size; ++i)
	{
		keyVals.push_back(startKey);
		do
		{ // ensure unique keys
			randVal = distribution(generator);
		} while (std::find(usedRandValues.begin(), usedRandValues.end(), randVal) != usedRandValues.end());
		keyFunc(startKey, randVal);
	}

	return std::move(keyVals);
}

}