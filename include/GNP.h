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

	inline explicit gnuplot(filepath_t);

	gnuplot(const gnuplot&) = delete;
	inline gnuplot(gnuplot&&);

	gnuplot& operator=(const gnuplot&) = delete;
	inline gnuplot& operator=(gnuplot&&);

	inline ~gnuplot() noexcept;

	template<typename... Ts> void operator() (Ts&&...);

	inline void name_axis(
		const std::string& = "x-axis",
		const std::string& = "y-axis",
		const std::string& = "z-axis"
	);

	inline void set_terminal_png(const uint32_t, const uint32_t);

	inline void set_terminal_window(const uint32_t, const uint32_t);

	inline void set_png_filename(filepath_t);

	inline void add_linestyle(
		const uint32_t,
		const std::string&,
		const uint32_t = 2,
		const uint32_t = 1,
		const uint32_t = 2,
		const float = 1.5f
	);

	inline void add_grid();

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
			("  ", point.first, " ", point.second, '\n');

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

#include <stdexcept>

#ifdef _WIN32
#define PIPE_OPEN _popen
#define PIPE_CLOSE _pclose
#else
#define PIPE_OPEN popen
#define PIPE_CLOSE pclose
#endif

gnuplot::gnuplot(filepath_t filepath)
{
	//gnuplot_path += "\\gnuplot -persist";
	_gnuplot_pipe = PIPE_OPEN(filepath, "w");

	if (_gnuplot_pipe == NULL) // PIPE_OPEN itself failed
		throw std::runtime_error("PIPE_OPEN failed");

	if (PIPE_CLOSE(_gnuplot_pipe) != 0) // command failed
		throw std::runtime_error("gnuplot filepath not found");

	_gnuplot_pipe = PIPE_OPEN(filepath, "w");
}

gnuplot::gnuplot(gnuplot&& other) :
	_gnuplot_pipe(std::move(other._gnuplot_pipe))
{
	other._gnuplot_pipe = nullptr; // indicate that it was moved
}

gnuplot& gnuplot::operator=(gnuplot&& other)
{
	if (this != std::addressof(other))
	{
		_gnuplot_pipe = std::move(other._gnuplot_pipe);
		other._gnuplot_pipe = nullptr; // indicate that it was moved
	}
	return *this;
}

gnuplot::~gnuplot() noexcept
{
	if (_gnuplot_pipe != nullptr)
	{
		fprintf(_gnuplot_pipe, "exit\n");
		PIPE_CLOSE(_gnuplot_pipe);
	}
}

void gnuplot::name_axis(
	const std::string& x_label,
	const std::string& y_label,
	const std::string& z_zabel
)
{
	operator()("set xlabel \"" + x_label + "\"");
	operator()("set ylabel \"" + y_label + "\"");
	operator()("set zlabel \"" + z_zabel + "\"");
}

void gnuplot::set_terminal_png(
	const uint32_t horizontal_res,
	const uint32_t vertical_res
)
{
	operator()(
		"set terminal pngcairo enhanced font 'Verdana,12' background rgb '#FCFCFC' size ",
		horizontal_res, ", ", vertical_res
	);
}

void gnuplot::set_terminal_window(
	const uint32_t horizontal_res,
	const uint32_t vertical_res
)
{
	operator()(
		"set terminal wxt enhanced font 'Verdana,12' background rgb '#FCFCFC' size ",
		horizontal_res, ", ", vertical_res
	);
}

void gnuplot::set_png_filename(filepath_t filename)
{
	// subdirectorys do not work
	operator()("set output \"", filename, ".png\"");
}

void gnuplot::add_linestyle(
	const uint32_t index,
	const std::string& color,
	const uint32_t linewidth,
	const uint32_t linetype,
	const uint32_t pointtype,
	const float pointsize
)
{
	operator()(
		"set style line ", index,
		" lc rgb \"" + color +
		"\" lw ", linewidth,
		" dashtype ", linetype,
		" pt ", pointtype,
		" ps ", pointsize
	);
}

void gnuplot::add_grid()
{
	operator()("set style line 11 lc rgb '#4F4A4A' dashtype 1 lw 1");
	operator()("set border 3 back ls 11");
	operator()("set style line 12 lc rgb '#636161' dashtype 3 lw 1");
	operator()("set grid back ls 12");
}

}

#undef PIPE_OPEN
#undef PIPE_CLOSE

#endif // VOOL_GNP_H_INCLUDED