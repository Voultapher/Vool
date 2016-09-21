/*
* Vool - Generic benchmark test suit using GNP to visualize results
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
#include <cassert>

#include <chrono>
#include <random>

namespace vool
{

namespace suithelper
{
	using plot_t = plot_data_2D<int64_t>;
	using point_t = plot_t::point_t;
	using graph_t = std::vector<plot_t>;

	struct result_t
	{
		suithelper::graph_t graph;
		std::string category_name;

		explicit result_t(
			suithelper::graph_t g,
			std::string n
		)
			: graph(std::move(g)), category_name(std::move(n))
		{ }

		result_t(const result_t&) = delete;
		result_t(result_t&&) = default;

		result_t& operator= (const result_t&) = delete;
		result_t& operator= (result_t&&) = default;
	};
}

struct suit_configuration
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

	explicit suit_configuration()
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
	using result_t = suithelper::result_t;

	explicit TestSuit(
		const suit_configuration&,
		TestCategorys&...
	);

	TestSuit(const TestSuit&) = delete;
	TestSuit(TestSuit<TestCategorys...>&&) = default;

	TestSuit& operator=(TestSuit<TestCategorys...>&&) = default;
	TestSuit& operator=(const TestSuit&) = delete;

	void runAllTests(const size_t, const size_t);

	void renderCategory(const result_t&);

	void renderResults();

private:
	std::tuple<TestCategorys...> _categorys;
	
	std::vector<result_t> _results;

	suit_configuration _suitConfiguration;

	gnuplot _gnuplot;

	bool isValidGraph(const suithelper::graph_t&);

	void pipeResult(const result_t&);
};

namespace suithelper
{
	template<typename Func> class Test
	{
	public:
		explicit Test(Func&, const std::string&);

		void setInvisible() { _visible = false; }

		point_t runTest(const size_t, const size_t);

		const std::string& getName() const { return _name; }

		const bool isVisible() const { return _visible; }

	private:
		Func _testFunc;
		std::string _name;
		bool _visible;

		inline const auto timerStart() const
		{
			return std::chrono::high_resolution_clock::now();
		}

		inline point_t timerEnd(
			const std::chrono::high_resolution_clock::time_point&,
			const size_t,
			const size_t
		);
	};

	template<typename... Tests> class TestCategory
	{
	public:
		explicit TestCategory(const std::string& categoryName, Tests&... tests)
			: _categoryName(categoryName), _tests(std::make_tuple(tests...))
		{ }

		graph_t runTestRange(
			const size_t,
			const size_t,
			const size_t,
			const size_t
		);

		const std::string& name() const { return _categoryName; }

	private:
		std::tuple<Tests...> _tests;
		std::string _categoryName;
	};
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

template<typename T> std::vector<T> generateContainer(ContainerConfig<T>);

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

	// --- Test ---

	template<typename T> Test<T>::Test(
		T& func,
		const std::string& name
	)
		:
		_testFunc(func),
		_name(name),
		_visible(true)
	{ }

	template<typename T> inline suithelper::point_t Test<T>::timerEnd(
		const std::chrono::high_resolution_clock::time_point& start,
		const size_t iterations,
		const size_t repCount
	)
	{
		auto end = std::chrono::high_resolution_clock::now();

		assert(repCount != 0 && "Repeating task 0 times!");
		int64_t deltaTimeNano = std::chrono::duration_cast<std::chrono::nanoseconds>
			(end - start).count() / repCount;

		return{ static_cast<int64_t>(iterations), deltaTimeNano };
	}

	template<typename T> suithelper::point_t Test<T>::runTest(
		const size_t size,
		const size_t repCount
	)
	{
		auto start = timerStart();
		for (size_t i = 0; i < repCount; ++i)
			_testFunc(size);
		return timerEnd(start, size, repCount);
	}

	// --- TestCategory ---

	template<typename... Ts> graph_t TestCategory<Ts...>::runTestRange(
		const size_t minSize,
		const size_t maxSize,
		const size_t stepCount,
		const size_t repCount
	)
	{
		std::vector<
			std::pair<
				std::vector<point_t>,
				std::string
			>
		> graph_plots;

		for_each_in_tuple(_tests,
			[&graph_plots, stepCount](auto& test)
			{
				graph_plots.emplace_back(
					std::vector<point_t>{},
					test.getName()
				);
				graph_plots.back().first.reserve(stepCount + 2);
			}
		);

		auto runCategoryTests = [this, &graph_plots, &repCount]
		(const size_t size)
		{
			auto graph_plots_it = graph_plots.begin();

			for_each_in_tuple(_tests,
				[&graph_plots_it, size, repCount](auto& test)
				{
					auto result = test.runTest(size, repCount);

					if (test.isVisible())
					{
						graph_plots_it->first.push_back(result);
						++graph_plots_it;
					}
				}
			);
		};

		for (size_t size = minSize; size < maxSize; size += 1 + maxSize / stepCount)
			runCategoryTests(size);

		runCategoryTests(maxSize);

		std::sort(graph_plots.begin(), graph_plots.end(),
			[](const auto& a, const auto& b)
			{ return a.first.back().second > b.first.back().second; }
		);

		uint32_t index = {};
		graph_t category_graph;

		std::for_each(graph_plots.begin(), graph_plots.end(),
			[&category_graph, &index](const auto& plots)
			{ category_graph.emplace_back(std::move(plots.first), index++, plots.second); }
		);

		return category_graph;
	}
}

// --- TestSuit ---

template<typename... Ts> TestSuit<Ts...>::TestSuit(
	const suit_configuration& suitConfiguration,
	Ts&... categorys
) :
	_suitConfiguration(suitConfiguration),
	_categorys(std::make_tuple(categorys...)),
	_gnuplot(_suitConfiguration.gnuplotPath, _suitConfiguration.persistent) // may throw
{
	_gnuplot.set_terminal_window(_suitConfiguration.xResolution, _suitConfiguration.yResolution);

	_gnuplot << "set samples 500";
	_gnuplot.add_linestyle(1, "#FF5A62", 2, 3, 5, 1.5f);
	_gnuplot.add_linestyle(2, "#2E9ACC", 2, 3, 6, 1.5f);
	_gnuplot.add_linestyle(3, "#9871FF", 2, 3, 7, 1.5f);
	_gnuplot.add_linestyle(4, "#E8803A", 2, 3, 8, 1.5f);
	_gnuplot.add_linestyle(5, "#46E86C", 2, 3, 9, 1.5f);
	_gnuplot.add_grid();
	_gnuplot.name_axis(_suitConfiguration.xAxisName, _suitConfiguration.yAxisName);
}

template<typename... Ts> void TestSuit<Ts...>::runAllTests(
	const size_t minSize,
	const size_t maxSize
)
{
	if (maxSize == 0 || maxSize < minSize)
		return;

	_results.clear();

	suithelper::for_each_in_tuple(_categorys,
		[
			minSize,
			maxSize,
			&results = _results,
			stepCount = _suitConfiguration.stepCount,
			repCount = _suitConfiguration.repCount
		](auto& category)
		{
			results.emplace_back(
				category.runTestRange(minSize, maxSize, stepCount, repCount),
				category.name()
			);
		}
	);
}

template<typename... Ts> bool TestSuit<Ts...>::isValidGraph(
	const suithelper::graph_t& graph
)
{
	if (graph.size() == 0)
		return false;

	size_t plotSize = graph.back().points().size();
	for (const auto& plot : graph)
		if (plot.points().size() == 0 || plot.points().size() != plotSize)
			return false;

	return true;
}

template<typename... Ts> void TestSuit<Ts...>::pipeResult(
	const result_t& result
)
{
	// if there are results, write them to a .dat file
	if (result.graph.size() == 0)
		return;

	_gnuplot.write_and_plot(
		result.graph,
		_suitConfiguration.resultDataPath + result.category_name + ".dat");

	if (_suitConfiguration.pngOutput)
	{
		// tell gnuplot to create a .png
		_gnuplot.set_terminal_png(
			_suitConfiguration.xResolution,
			_suitConfiguration.yResolution);
		_gnuplot.set_png_filename(_suitConfiguration.resultName + result.category_name);
		_gnuplot.plot(
			result.graph,
			_suitConfiguration.resultDataPath + result.category_name + ".dat");
	}
}

template<typename... Ts> void TestSuit<Ts...>::renderCategory(
	const result_t& result
)
{
	if (isValidGraph(result.graph))
	{
		pipeResult(result);
	}
	else if (_suitConfiguration.warningsActive)
	{
		std::cout
			<< "The category: \""
			<< result.category_name
			<< "\" had invalid plots!\n";
	}
}

template<typename... Ts> void TestSuit<Ts...>::renderResults()
{
	for (const auto& result : _results)
	{
		if (result.graph.size() > 0)
		{
			renderCategory(result);
		}
		else if (_suitConfiguration.warningsActive)
		{
			std::cout
				<< "The category: \""
				<< result.category_name
				<< "\" had 0 plots!\n";
		}
	}
}

// --- make funcitons ---

template<typename T> suithelper::Test<T> createTest(
	const std::string& testName, T func
)
{
	return suithelper::Test<T>(std::ref(func), testName);
}

template<typename... Ts> suithelper::TestCategory<Ts...>createTestCategory(
	const std::string& categoryName, Ts&... tests
)
{
	return suithelper::TestCategory<Ts...>(categoryName, tests...);
}

template <typename... Ts> TestSuit<Ts...> createTestSuit(
	const suit_configuration& suitConfiguration, Ts&... categorys
)
{
	return TestSuit<Ts...>(suitConfiguration, categorys...);
}

// --- generateContainer ---

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