/*
* Vool - Generic benchmark test suit using GNP to visualize results
*
* Copyright (c) 2016 Lukas Bergdoll - www.lukas-bergdoll.net
*
* This code is licensed under the Apache License 2.0 (https://opensource.org/licenses/Apache-2.0)
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

namespace test_suit_util
{
	using plot_t = plot_data_2D<int64_t>;
	using point_t = plot_t::point_t;
	using graph_t = std::vector<plot_t>;

	class result_t;
}

struct suit_config
{
	uint32_t x_res;
	uint32_t y_res;
	bool warnings_active;
	bool png_output;
	bool persistent;
	size_t steps;
	size_t repetitions;
	gnuplot::filepath_t gnuplot_path;
	gnuplot::filepath_t output_filepath;
	gnuplot::filepath_t filename;
	std::string x_name;
	std::string y_name;

	explicit suit_config()
		: x_res(1000), y_res(500),
		warnings_active(true),
		png_output(true),
		persistent(false),
		steps(20),
		repetitions(3),
		gnuplot_path("C:\\ProgramData\\gnuplot\\bin\\gnuplot"),
		output_filepath(""),
		filename("Result"),
		x_name("Size"), y_name("Full Time in nanoseconds")
	{}
};

template<typename... TestCategorys> class test_suit
{
public:
	using result_t = test_suit_util::result_t;

	explicit test_suit(
		const suit_config&,
		TestCategorys&&...
	);

	test_suit(const test_suit&) = delete;
	test_suit(test_suit&&) = default;

	test_suit& operator=(const test_suit&) = delete;
	test_suit& operator=(test_suit&&) = default;

	void perform_categorys(const size_t, const size_t);

	void render_category(const result_t&);

	void render_results();

private:
	std::tuple<TestCategorys...> _categorys;
	
	std::vector<result_t> _results;

	suit_config _suit_config;

	gnuplot _gnuplot;

	bool valid_graph(const test_suit_util::graph_t&);

	void pipe_result(const result_t&);
};

namespace test_suit_util
{
class result_t
{
public:
	graph_t graph;
	std::string category_name;

	explicit result_t(
		graph_t g,
		std::string n
	)
		: graph(std::move(g)), category_name(std::move(n))
	{ }

	result_t(const result_t&) = delete;
	result_t(result_t&&) = default;

	result_t& operator= (const result_t&) = delete;
	result_t& operator= (result_t&&) = default;
};

template<typename Func> class test
{
public:
	explicit test(const std::string&, Func&&);

	test(const test&) = default;
	test(test&&) = default;

	test& operator= (const test&) = default;
	test& operator= (test&&) = default;

	point_t run_test(const size_t, const size_t);

	void flag_invisible() { _visible = false; }

	const std::string& name() const { return _name; }
	const bool visible() const { return _visible; }

private:
	Func _func;
	std::string _name;
	bool _visible;

	inline const auto start_timer() const
	{
		return std::chrono::high_resolution_clock::now();
	}

	inline point_t stop_timer(
		const std::chrono::high_resolution_clock::time_point&,
		const size_t,
		const size_t
	);
};

template<typename... Tests> class test_category
{
public:
	explicit test_category(const std::string&, Tests&&...);

	test_category(const test_category&) = default;
	test_category(test_category&&) = default;

	test_category& operator= (const test_category&) = default;
	test_category& operator= (test_category&&) = default;

	graph_t perform_tests(
		const size_t,
		const size_t,
		const size_t,
		const size_t
	);

	const std::string& name() const { return _name; }

private:
	std::tuple<Tests...> _tests;
	std::string _name;

	using graph_plots_t = std::vector<std::pair<
		std::vector<point_t>, std::string>
	>;

	graph_plots_t build_graph_plots(const size_t);

	void benchmark_size(const size_t, const size_t, graph_plots_t&);
};
}

template<typename T> struct ContainerConfig
{
	using type = T;

	size_t size; // resulting size of the container
	T lower_bound; // lowest possible value in container
	T upper_bound; // highest possible value in container
	bool unique; // true should make all values unique

	ContainerConfig() :
		size(0),
		lower_bound(std::numeric_limits<T>::min()),
		upper_bound(std::numeric_limits<T>::max()),
		unique(true)
	{
		static_assert(std::is_arithmetic<T>::value, "Type T has to be arithmetic!");
	}
};

template<typename T> std::vector<T> generate_container(ContainerConfig<T>);

// ----- IMPLEMENTATION -----

namespace test_suit_util
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


// --- test ---

template<typename T> test<T>::test(
	const std::string& name,
	T&& func
)
	:
	_name(name),
	_func(std::forward<T>(func)),
	_visible(true)
{ }

template<typename T> inline test_suit_util::point_t test<T>::stop_timer(
	const std::chrono::high_resolution_clock::time_point& start,
	const size_t iterations,
	const size_t repetitions
)
{
	auto end = std::chrono::high_resolution_clock::now();

	assert(repetitions != 0 && "Repeating task 0 times!");
	int64_t delta_time = std::chrono::duration_cast<std::chrono::nanoseconds>
		(end - start).count() / repetitions;

	return{ static_cast<int64_t>(iterations), delta_time };
}

template<typename T> test_suit_util::point_t test<T>::run_test(
	const size_t size,
	const size_t repetitions
)
{
	auto start = start_timer();
	for (size_t i = 0; i < repetitions; ++i)
		_func(size);
	return stop_timer(start, size, repetitions);
}

// --- test_category ---

template<typename... Ts> test_category<Ts...>::test_category(
	const std::string& categoryName,
	Ts&&... tests
)
	: _name(categoryName),
	_tests(std::forward<Ts>(tests)...)
{ }

template<typename... Ts> auto test_category<Ts...>::build_graph_plots(
	const size_t reserve_size
) -> graph_plots_t
{
	graph_plots_t graph_plots;

	for_each_in_tuple(_tests,
		[&](auto& test)
		{
			graph_plots.emplace_back(
				std::vector<point_t>{},
				test.name()
			);
			graph_plots.back().first.reserve(reserve_size);
		}
	);

	return graph_plots;
}

template<typename... Ts> void test_category<Ts...>::benchmark_size(
	const size_t size,
	const size_t repetitions,
	graph_plots_t& graph_plots
)
{
	auto graph_plots_it = graph_plots.begin();

	for_each_in_tuple(_tests,
		[&graph_plots_it, size, repetitions](auto& test)
		{
			auto result = test.run_test(size, repetitions);

			if (test.visible())
			{
				graph_plots_it->first.push_back(result);
				++graph_plots_it;
			}
		}
	);
};

template<typename... Ts> graph_t test_category<Ts...>::perform_tests(
	const size_t min,
	const size_t max,
	const size_t steps,
	const size_t repetitions
)
{
	auto graph_plots = build_graph_plots(steps + 2);

	for (size_t size = min; size < max; size += 1 + max / steps)
		benchmark_size(size, repetitions, graph_plots);

	benchmark_size(max, repetitions, graph_plots);

	std::sort(graph_plots.begin(), graph_plots.end(),
		[](const auto& a, const auto& b)
		{ return a.first.back().second > b.first.back().second; }
	);

	uint32_t index = {};
	graph_t category_graph;

	std::for_each(graph_plots.begin(), graph_plots.end(),
		[&category_graph, &index](const auto& plot)
		{ category_graph.emplace_back(std::move(plot.first), index++, plot.second); }
	);

	return category_graph;
}

}

// --- test_suit ---

template<typename... Ts> test_suit<Ts...>::test_suit(
	const suit_config& suit_configuration,
	Ts&&... categorys
) :
	_suit_config(suit_configuration),
	_categorys(std::forward<Ts>(categorys)...),
	_gnuplot(_suit_config.gnuplot_path)
{
	_gnuplot.set_terminal_window(_suit_config.x_res, _suit_config.y_res);

	_gnuplot("set samples 500");
	_gnuplot.add_linestyle(1, "#FF5A62", 2, 3, 5, 1.5f);
	_gnuplot.add_linestyle(2, "#2E9ACC", 2, 3, 6, 1.5f);
	_gnuplot.add_linestyle(3, "#9871FF", 2, 3, 7, 1.5f);
	_gnuplot.add_linestyle(4, "#E8803A", 2, 3, 8, 1.5f);
	_gnuplot.add_linestyle(5, "#46E86C", 2, 3, 9, 1.5f);
	_gnuplot.add_grid();
	_gnuplot.name_axis(_suit_config.x_name, _suit_config.y_name);
}

template<typename... Ts> void test_suit<Ts...>::perform_categorys(
	const size_t min,
	const size_t max
)
{
	if (max == 0 || max < min)
		return;

	_results.clear();

	test_suit_util::for_each_in_tuple(_categorys,
		[
			min,
			max,
			&results = _results,
			steps = _suit_config.steps,
			repetitions = _suit_config.repetitions
		](auto& category)
		{
			results.emplace_back(
				category.perform_tests(min, max, steps, repetitions),
				category.name()
			);
		}
	);
}

template<typename... Ts> bool test_suit<Ts...>::valid_graph(
	const test_suit_util::graph_t& graph
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

template<typename... Ts> void test_suit<Ts...>::pipe_result(
	const result_t& result
)
{
	if (result.graph.size() == 0)
		return;

	std::string filename(_suit_config.output_filepath + result.category_name + ".dat");

	_gnuplot.write_and_plot(
		result.graph,
		filename.c_str()
	);

	if (_suit_config.png_output)
	{
		_gnuplot.set_terminal_png(_suit_config.x_res, _suit_config.y_res);
		_gnuplot.set_png_filename(
			std::string(_suit_config.filename + result.category_name).c_str()
		);
		_gnuplot.plot(
			result.graph,
			filename.c_str()
		);
	}
}

template<typename... Ts> void test_suit<Ts...>::render_category(
	const result_t& result
)
{
	if (valid_graph(result.graph))
	{
		pipe_result(result);
	}
	else if (_suit_config.warnings_active)
	{
		std::cout
			<< "The category: \""
			<< result.category_name
			<< "\" had invalid plots!\n";
	}
}

template<typename... Ts> void test_suit<Ts...>::render_results()
{
	for (const auto& result : _results)
	{
		if (result.graph.size() > 0)
		{
			render_category(result);
		}
		else if (_suit_config.warnings_active)
		{
			std::cout
				<< "The category: \""
				<< result.category_name
				<< "\" had 0 plots!\n";
		}
	}
}

// --- make funcitons ---

template<typename T> test_suit_util::test<T> make_test(
	const std::string& testName, T func
)
{
	return test_suit_util::test<T>(testName, std::forward<T>(func));
}

template<typename... Ts> test_suit_util::test_category<Ts...>make_test_category(
	const std::string& categoryName, Ts&&... tests
)
{
	return test_suit_util::test_category<Ts...>(
		categoryName,
		std::forward<Ts>(tests)...
	);
}

template <typename... Ts> test_suit<Ts...> make_test_suit(
	const suit_config& suit_configuration, Ts&&... categorys
)
{
	return test_suit<Ts...>(
		suit_configuration,
		std::forward<Ts>(categorys)...
	);
}

// --- generate_container ---

template<typename T> std::vector<T> generate_container(ContainerConfig<T> config)
{
	static_assert(std::is_arithmetic<T>::value, "Type T has to be arithmetic!");

	std::vector<T> ret(config.size);

	using distribution_t = std::conditional_t<
		std::is_floating_point<T>::value,
		std::uniform_real_distribution<T>,
		std::uniform_int_distribution<T>
	>;

	std::mt19937_64 generator(1580); // fixed seed for reproducibility

	distribution_t distribution(config.lower_bound, config.upper_bound);

	if (config.unique)
	{
		assert(
			!(std::is_integral<T>::value
			&& config.upper_bound - config.lower_bound < config.size)
			&& "container cannot be unique, given config boundarys"
		);

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