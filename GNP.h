/*
* Vool - gnuplot interface using popen/ _popen to pipe data to gnuplot
*
* Copyright (C) 2016 by Lukas Bergdoll - www.lukas-bergdoll.net
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
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
	explicit gnuplot(std::string, const bool = true);

	gnuplot(const gnuplot&) = delete;
	gnuplot(gnuplot&&);

	gnuplot& operator=(const gnuplot&) = delete;
	gnuplot& operator=(gnuplot&&);

	~gnuplot() noexcept;

	template<typename... Ts> void operator() (const Ts&...);

	void operator<< (std::string);

	void name_axis(
		const std::string& = "x-axis",
		const std::string& = "y-axis",
		const std::string& = "z-axis"
	);

	void set_terminal_png(const uint32_t, const uint32_t);

	void set_terminal_window(const uint32_t, const uint32_t);

	void set_png_filename(const std::string&);

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
		const std::string& = "data.dat"
	);

	template<typename T> void write(
		const std::vector<plot_data_2D<T>>&,
		const std::string& = "data.dat"
	);

	template<typename T> void plot(
		const std::vector<plot_data_2D<T>>&,
		const std::string&
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

namespace util
{
	// automatic string conversion from variadic pack
	inline std::string convert(const std::string& arg) { return arg; }
	inline std::string convert(const char* arg) { return arg; }

	// overload if T is not an string or const char*
	template<typename T> std::string convert(const T& arg)
	{
		return std::to_string(arg);
	}

	template<typename... Ts> std::string convert_to_string_v(const Ts&... args)
	{
		std::string command;
		auto wFunc = [&command](const auto& args)
		{
			command += convert(args);
		};
		static_cast<void>(std::initializer_list<int> { 0, (wFunc(args), 0)... });
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

template<typename... Ts> void gnuplot::operator() (const Ts&... args)
{
	// send pack of elements that if needed will get converted to string command to gnuplot
	auto command = util::convert_to_string_v(args...);
	command += "\n";

	fprintf(_gnuplot_pipe, "%s\n", command.c_str());
	fflush(_gnuplot_pipe);
}

template<typename T> void gnuplot::write_and_plot(
	const std::vector<plot_data_2D<T>>& plots,
	const std::string& filepath
)
{
	write(plots, filepath);
	plot(plots, filepath);
}

template<typename T> void gnuplot::write(
	const std::vector<plot_data_2D<T>>& plots,
	const std::string& filepath
)
{
	if (plots.size() == 0)
		return;

	// write data to filepath
	std::ofstream plot_file(filepath);

	if (!std::ifstream(filepath).good())
	{
		std::cerr << "File \"" << filepath.c_str() << "\" not found!\n";
		return;
	}

	for (const auto& plot : plots)
	{
		plot_file << util::convert_to_string_v("#(index ", plot.index(), ")\n");
		plot_file << "# X Y\n";
		for (const auto point : plot.points())
			plot_file << util::convert_to_string_v
			("  ", point.first, " ", point.second, "\n");

		plot_file << "\n\n";
	}
	plot_file.close();
}

template<typename T> void gnuplot::plot(
	const std::vector<plot_data_2D<T>>& plots,
	const std::string& filepath
)
{
	if (!std::ifstream(filepath).good())
	{
		std::cerr << "File \"" << filepath.c_str() << "\" not found!\n";
		return;
	}

	// create command and push it to gnuplot
	std::string command = "plot '" + filepath + "' ";
	for (const auto& plot : plots)
	{
		command += util::convert_to_string_v
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