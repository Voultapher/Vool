/*
* Vool - Generic perfomance test suit using GNP to visualize results
*
* Copyright (C) 2016 by Lukas Bergdoll - www.lukas-bergdoll.net
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#ifndef VOOL_TESTSUIT_H_INCLUDED
#define VOOL_TESTSUIT_H_INCLUDED

#include "GNP.h"

#include <iostream>
#include <vector>
#include <algorithm>
#include <string>
#include <tuple>

#include <chrono>
#include <random>

namespace vool
{

struct Result
{
public:
	Result(const size_t size, const int64_t fT, const int64_t aT, const std::string& tN) :
		_size(size), _fullTime(fT), _averageTime(aT), _taskName(tN)
	{ }

	const size_t getSize() const { return _size; }
	const int64_t getFullTime() const { return _fullTime; }
	const int64_t getAverageTime() const { return _averageTime; }
	const std::string& getTaskName() const { return _taskName; }

private:
	size_t _size;
	int64_t _fullTime;
	int64_t _averageTime;
	std::string _taskName;
};

template<typename TestFunc> class Test
{
public:
	explicit Test(TestFunc& func, const std::string& testName)
		:
		_testFunc(func),
		_testName(testName),
		_result(0, 0, 0, "No Test"),
		_visible(true)
	{ }

	void setInvisible() { _visible = false; }

	void runTest(const size_t, const size_t);

	const bool isVisible() const { return _visible; }
	const Result& getResult() const { return _result; }

private:
	TestFunc _testFunc;
	Result _result;
	std::string _testName;
	bool _visible;

	inline const auto timerStart() const
	{
		return std::chrono::high_resolution_clock::now();
	}

	inline void timerEnd(
		const std::chrono::high_resolution_clock::time_point& start,
		const size_t iterations,
		const size_t repCount
	);
};

template<typename Func>
Test<Func> createTest(const std::string& testName, Func func)
{
	return Test<Func>(std::ref(func), testName);
}

template<typename... Tests> class TestCategory
{
public:
	using res_t = std::vector<std::vector<Result>>;

	explicit TestCategory(const std::string& categoryName, Tests&... tests)
		: _categoryName(categoryName), _tests(std::make_tuple(tests...))
	{ }

	void runTestRange(
		const size_t,
		const size_t,
		const size_t,
		const size_t
	);

	const std::pair<res_t, std::string> getResults() const
	{
		return std::make_pair(_results, _categoryName);
	};

private:
	std::tuple<Tests...> _tests;
	res_t _results;
	std::string _categoryName;
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
	bool persistent;
	size_t stepCount;
	size_t repCount;
	std::string gnuplotPath;
	std::string xAxisName;
	std::string yAxisName;
	std::string resultDataPath;
	std::string resultName;

	explicit SuitConfiguration()
		: xResolution(1000), yResolution(500),
		warningsActive(true),
		pngOutput(true),
		persistent(false),
		stepCount(20),
		repCount(3),
		gnuplotPath("C:\\ProgramData\\gnuplot\\bin"),
		xAxisName("Size"), yAxisName("Full Time in nanoseconds"),
		resultDataPath(""), resultName("Result") { }
};


template<typename... TestCategorys> class TestSuit
{
public:
	using plots_t = std::vector<PlotData2D<double>>;
	using category_res_t = std::pair<std::vector<std::vector<Result>>, std::string>;
	using res_t = std::vector<category_res_t>;

	// constructor
	explicit TestSuit(
		const SuitConfiguration&,
		TestCategorys&...
	);

	// move constructor
	TestSuit(TestSuit<TestCategorys...>&& other) = default;

	// move operator
	TestSuit& operator=(TestSuit<TestCategorys...>&& other) = default;

	// no copy constructor
	TestSuit(const TestSuit& other) = delete;

	// no copy operator
	TestSuit& operator=(const TestSuit& other) = delete;

	void runAllTests(const size_t, const size_t);

	void renderCategory(const category_res_t&);

	void renderResults();

	const res_t& getResults() const { return _results; }

private:
	std::tuple<TestCategorys...> _categorys;
	res_t _results;

	SuitConfiguration _suitConfiguration;

	Gnuplot _plot;

	plots_t createPlots(const category_res_t&);

	bool areValidPlots(const plots_t&);

	void sortResult(plots_t&);

	void pipeResult(const plots_t&, const std::string&);
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

template<typename T> std::vector<T> generateContainer(ContainerConfig<T> config);

// ----- IMPLEMENTATION -----

namespace suithelper
{
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
}

// --- Test ---

template<typename T> inline void Test<T>::timerEnd(
	const std::chrono::high_resolution_clock::time_point& start,
	const size_t iterations,
	const size_t repCount
)
{
	auto end = std::chrono::high_resolution_clock::now(); // save end time

	int64_t deltaTimeNano = std::chrono::duration_cast<std::chrono::nanoseconds>
		(end - start).count() / repCount; // calculate difference in nanosecons

											// calculate how long each iteration took on average
	int64_t averageIndividualTime = deltaTimeNano / iterations;

	_result = std::move(Result(iterations, deltaTimeNano, averageIndividualTime, _testName));
}

template<typename T> void Test<T>::runTest(const size_t size, const size_t repCount)
{
	auto start = timerStart();
	for (size_t i = 0; i < repCount; ++i)
		_testFunc(size);
	timerEnd(start, size, repCount);
}

// --- TestCategory ---

template<typename... Tests> void TestCategory<Tests...>::runTestRange(
	const size_t minSize,
	const size_t maxSize,
	const size_t stepCount,
	const size_t repCount
)
{
	// all test will be executed in construction order

	// size construct to fit the amount of tests
	std::vector<std::vector<Result>> categoryResults(sizeof...(Tests));

	auto runCategoryTests = [this, &categoryResults, &repCount]
	(const size_t size)
	{
		auto categoryResults_it = categoryResults.begin();

		suithelper::for_each_in_tuple(_tests,
			[&categoryResults_it, size, repCount, &categoryResults](auto& element)
		{
			element.runTest(size, repCount);

			if (element.isVisible())
			{
				categoryResults_it->push_back(element.getResult());
				++categoryResults_it;
			}
		}
		);
	};


	size_t size = minSize == 0 ? 1 : minSize;
	for (; size < maxSize; size += 1 + maxSize / stepCount)
		runCategoryTests(size);

	runCategoryTests(maxSize);

	categoryResults.shrink_to_fit();

	_results = std::move(categoryResults);
}

// --- TestSuit ---

template<typename... TestCategorys> TestSuit<TestCategorys...>::TestSuit(
	const SuitConfiguration& suitConfiguration,
	TestCategorys&... categorys
) :
	_suitConfiguration(suitConfiguration),
	_categorys(std::make_tuple(categorys...)),
	_plot(_suitConfiguration.gnuplotPath, _suitConfiguration.persistent) // may throw
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
}

template<typename... TestCategorys> void TestSuit<TestCategorys...>::runAllTests(
	const size_t minSize,
	const size_t maxSize
)
{
	// all test will be executed in construction order

	if (maxSize == 0 || maxSize < minSize)
		return;

	_results.clear();
	auto& categoryResults = _results;

	// for every category
	suithelper::for_each_in_tuple(_categorys,
		[
			minSize,
			maxSize,
			&categoryResults,
			stepCount = _suitConfiguration.stepCount,
			repCount = _suitConfiguration.repCount
		](auto& element)
	{
		element.runTestRange(minSize, maxSize, stepCount, repCount);
		categoryResults.push_back(element.getResults());
	});
}

template<typename... TestCategorys> auto TestSuit<TestCategorys...>::createPlots(
	const category_res_t& category_res
) -> plots_t
{
	plots_t plots;
	plots.reserve(category_res.first.size());

	for (const auto& results : category_res.first)
	{
		if (results.size() == 0)
			continue;

		std::vector<std::pair<double, double>> points;
		points.reserve(results.size());
		for (const auto& result : results)
			points.push_back
			(
				std::make_pair(
					result.getSize(),
					result.getFullTime())
			);

		plots.emplace_back
		(
			std::move(points),
			results.back().getTaskName()
		);
	}

	return plots;
}

template<typename... TestCategorys> bool TestSuit<TestCategorys...>::areValidPlots(
	const plots_t& plots
)
{
	if (plots.size() == 0)
		return false;

	size_t plotSize = plots.back().getData().size();
	for (const auto& plot : plots)
		if (plot.getData().size() == 0 || plot.getData().size() != plotSize)
			return false;

	return true;
}

template<typename... TestCategorys> void TestSuit<TestCategorys...>::sortResult(
	plots_t& plots
)
{
	// sort test results top to bottom
	std::sort(plots.begin(), plots.end(),
		[](const auto& pointsA, const auto& pointsB)
	{
		return pointsA.getData().back() > pointsB.getData().back();
	});

	for (size_t i = 0; i < plots.size(); ++i)
	{
		plots[i].setIndexAndLineStyle(
			static_cast<uint32_t>(i),
			static_cast<uint32_t>(i) + 1
		);
	}
}

template<typename... TestCategorys> void TestSuit<TestCategorys...>::pipeResult(
	const plots_t& plots,
	const std::string& categoryName
)
{
	// if there are results, write them to a .dat file
	if (plots.size() == 0)
		return;

	_plot.writeAndPlotData(
		plots,
		_suitConfiguration.resultDataPath + categoryName + ".dat");

	if (_suitConfiguration.pngOutput)
	{
		// tell gnu_plot to create a .png
		_plot.setSaveMode(
			_suitConfiguration.xResolution,
			_suitConfiguration.yResolution);
		_plot.setOutput(_suitConfiguration.resultName + categoryName);
		_plot.plotData(
			plots,
			_suitConfiguration.resultDataPath + categoryName + ".dat");
	}
}

template<typename... TestCategorys> void TestSuit<TestCategorys...>::renderCategory(
	const category_res_t& category_res
)
{
	auto plots = createPlots(category_res);

	if (areValidPlots(plots))
	{
		sortResult(plots);
		pipeResult(plots, category_res.second);
	}
	else if (_suitConfiguration.warningsActive)
	{
		std::cout
			<< "The category: \""
			<< category_res.second
			<< "\" had invalid plots!\n";
	}
}

template<typename... TestCategorys> void TestSuit<TestCategorys...>::renderResults()
{
	for (const auto& category_res : _results)
		if (category_res.first.size() == 0)
		{
			if (_suitConfiguration.warningsActive)
				std::cout
					<< "The category: \""
					<< category_res.second
					<< "\" had 0 results!\n";
		}
		else
			renderCategory(category_res);
}

// --- else ---

template<typename T> std::vector<T> generateContainer(ContainerConfig<T> config)
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
		std::generate(ret.begin(), ret.end(), [&]() { return distribution(generator); });

	return ret;
}

}

#endif // VOOL_TESTSUIT_H_INCLUDED