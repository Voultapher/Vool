/*
* Vool - gnuplot interface using popen to pipe data to gnuplot
*
* Copyright (c) 2016 Lukas Bergdoll - www.lukas-bergdoll.net
*
* This code is licensed under the Apache License 2.0 (https://opensource.org/licenses/Apache-2.0)
*/

#ifndef VOOL_GNP_H_INCLUDED
#define VOOL_GNP_H_INCLUDED

#include <vector>
#include <string>
#include <fstream>
#include <iostream>

namespace vool
{

template<typename T> struct plot_data_2D;

class gnuplot
{
public:
	using filepath_t = const char*;

	explicit gnuplot(filepath_t);

	gnuplot(const gnuplot&) = delete;
	gnuplot(gnuplot&&);

	gnuplot& operator=(const gnuplot&) = delete;
	gnuplot& operator=(gnuplot&&);

	~gnuplot() noexcept;

	template<typename... Ts> void operator() (Ts&&...);

	void name_axis(
		const std::string& = "x-axis",
		const std::string& = "y-axis",
		const std::string& = "z-axis"
	);

	void set_terminal_png(const uint32_t, const uint32_t);

	void set_terminal_window(const uint32_t, const uint32_t);

	void set_png_filename(filepath_t);

	void add_linestyle(
		const uint32_t,
		const std::string&,
		const uint32_t = 2,
		const uint32_t = 1,
		const uint32_t = 2,
		const float = 1.5f
	);

	void add_grid();

	template<typename T> void write_and_plot(
		const std::vector<plot_data_2D<T>>&,
		filepath_t
	);

	template<typename T> void write(
		const std::vector<plot_data_2D<T>>&,
		filepath_t
	);

	template<typename T> void plot(
		const std::vector<plot_data_2D<T>>&,
		filepath_t
	);

private:
	FILE* _gnuplot_pipe;
};

template<typename T> struct plot_data_2D
{
public:
	using point_t = std::pair<T, T>; // first = x second = y

	explicit plot_data_2D(
		const std::vector<point_t>,
		const uint32_t,
		const std::string&
	);

	const auto& points() const { return _points; }
	const auto linestyle() const { return _linestyle; }
	const auto index() const { return _index; }
	const auto& name() const { return _name; }

private:
	std::vector<point_t> _points;
	uint32_t _linestyle;
	uint32_t _index;
	std::string _name;
};

// ----- IMPLEMENTATION -----

namespace gnuplot_util
{
	// automatic string conversion from variadic pack
	inline void cat(std::string& ret, const std::string& arg) { ret += arg; }
	inline void cat(std::string& ret, const char* arg) { ret += arg; }

	// overload if T is not an string or const char*
	template<typename T> void cat(std::string& ret, const T& arg)
	{
		ret += std::to_string(arg);
	}

	template<typename... Ts> std::string concatenate(Ts&&... args)
	{
		std::string command;
		auto wFunc = [&command](auto&& arg)
		{
			cat(command, std::forward<decltype(arg)>(arg));
		};

		static_cast<void>(std::initializer_list<int>
		{
			0, (wFunc(std::forward<Ts>(args)), 0)...
		});
		return command;
	}
}

// --- plot_data_2D ---

template<typename T> plot_data_2D<T>::plot_data_2D(
	const std::vector<point_t> points,
	const uint32_t index,
	const std::string& name
)
	: _points(points), _linestyle(index + 1), _index(index), _name(name)
{ }

// --- gnuplot ---

template<typename... Ts> void gnuplot::operator() (Ts&&... args)
{
	std::string command = gnuplot_util::concatenate(std::forward<Ts>(args)...);

	fprintf(_gnuplot_pipe, "%s\n", command.c_str());
	fflush(_gnuplot_pipe);
}

template<typename T> void gnuplot::write_and_plot(
	const std::vector<plot_data_2D<T>>& plots,
	filepath_t filepath
)
{
	write(plots, filepath);
	plot(plots, filepath);
}

template<typename T> void gnuplot::write(
	const std::vector<plot_data_2D<T>>& plots,
	filepath_t filepath
)
{
	if (plots.size() == 0)
		return;

	// write data to filepath
	std::ofstream plot_file(filepath);

	if (!std::ifstream(filepath).good())
	{
		std::cerr << "File \"" << filepath << "\" not found!\n";
		return;
	}

	for (const auto& plot : plots)
	{
		plot_file << gnuplot_util::concatenate("#(index ", plot.index(), ")\n");
		plot_file << "# X Y\n";
		for (const auto point : plot.points())
			plot_file << gnuplot_util::concatenate
			("  ", point.first, " ", point.second, "\n");

		plot_file << "\n\n";
	}
	plot_file.close();
}

template<typename T> void gnuplot::plot(
	const std::vector<plot_data_2D<T>>& plots,
	filepath_t filepath
)
{
	if (!std::ifstream(filepath).good())
	{
		std::cerr << "File \"" << filepath << "\" not found!\n";
		return;
	}

	// create command and push it to gnuplot
	std::string command = gnuplot_util::concatenate("plot '", filepath, "' ");

	for (const auto& plot : plots)
	{
		command += gnuplot_util::concatenate
		(
			"index ", plot.index(),
			" t '", plot.name(),
			"' with linespoints ls ", plot.linestyle(),
			", \'' "
		);
	}

	command.erase(command.size() - 5, 5); // remove the last ", \'' "

	operator()(command);
}

}

#endif // VOOL_GNP_H_INCLUDED