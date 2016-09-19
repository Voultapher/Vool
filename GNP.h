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

namespace vool
{

template<typename T> struct plot_data_2D
{
public:
	explicit plot_data_2D(
		const std::vector<std::pair<T, T>>,
		const std::string&
	);

	explicit plot_data_2D(
		const std::vector<std::pair<T, T>>,
		const uint32_t,
		const uint32_t,
		const std::string&
	);

	void set_index_and_linestyle(const uint32_t index, const uint32_t linestyle);

	const auto& points() const { return _points; }
	const auto linestyle() const { return _linestyle; }
	const auto index() const { return _index; }
	const auto& name() const { return _name; }

private:
	std::vector<std::pair<T, T>> _points;
	uint32_t _linestyle;
	uint32_t _index;
	std::string _name;
};

class gnuplot
{
public:
	explicit gnuplot(std::string, bool = true);

	gnuplot(gnuplot&&);

	gnuplot& operator=(gnuplot&&);

	gnuplot(const gnuplot&) = delete; // no copy constructor

	gnuplot& operator=(const gnuplot&) = delete; // no copy operator

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

// ----- IMPLEMENTATION -----

namespace util
{
	// automatic string conversion from variadic pack
	inline std::string convert(const std::string& arg) { return arg; }
	inline std::string convert(const char* arg) { return arg; }

	// overload if T is not an string or const char*
	template<typename T> std::string convert(const T& arg)
	{
		static_assert(!std::is_same<T, const char*>::value, "const char* buhu!");
		static_assert(!std::is_same<T, std::string>::value, "string buhu!");
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
	const std::vector<std::pair<T, T>> points,
	const std::string& name
)
	:_points(points), _name(name)
{ }

template<typename T> plot_data_2D<T>::plot_data_2D(
	const std::vector<std::pair<T, T>> points,
	const uint32_t linestyle,
	const uint32_t index,
	const std::string& name
)
	: _points(points), _linestyle(linestyle), _index(index), _name(name)
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

template<typename T> void plot_data_2D<T>::set_index_and_linestyle(
	const uint32_t index,
	const uint32_t linestyle
)
{
	_index = index;
	_linestyle = linestyle;
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
	if (plots.size() > 0)
	{
		// write data to filepath
		std::ofstream plot_file(filepath);

		if (plot_file.fail()) {
			std::perror(filepath.c_str());
			return;
		}

		for (const auto& plot : plots)
		{
			plot_file << util::convert_to_string_v("#(index ", plot.index(), ")\n");
			plot_file << "# X Y\n";
			for (const auto pointPair : plot.points())
				plot_file << util::convert_to_string_v
				("  ", pointPair.first, " ", pointPair.second, "\n");

			plot_file << "\n\n";
		}
		plot_file.close();
	}
}

template<typename T> void gnuplot::plot(
	const std::vector<plot_data_2D<T>>& plots,
	const std::string& filepath
)
{
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