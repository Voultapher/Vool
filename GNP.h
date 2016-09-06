/*
* Vool - Gnuplot interface using popen/ _popen to pipe data to gnuplot
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

template<typename T> struct PlotData2D
{
public:
	explicit PlotData2D(
		const std::vector<std::pair<T, T>>,
		const std::string&
	);

	explicit PlotData2D(
		const std::vector<std::pair<T, T>>,
		const uint32_t,
		const uint32_t,
		const std::string&
	);

	void setIndexAndLineStyle(const uint32_t index, const uint32_t linestyle);

	const auto& getData() const { return _points; }
	const auto getLinestyle() const { return _linestyle; }
	const auto getIndex() const { return _index; }
	const auto& getName() const { return _name; }

private:
	std::vector<std::pair<T, T>> _points;
	uint32_t _linestyle;
	uint32_t _index;
	std::string _name;
};

class Gnuplot
{
public:
	explicit Gnuplot(std::string, bool = true);

	Gnuplot(Gnuplot&&);

	Gnuplot& operator=(Gnuplot&&);

	Gnuplot(const Gnuplot&) = delete; // no copy constructor

	Gnuplot& operator=(const Gnuplot&) = delete; // no copy operator

	~Gnuplot() noexcept;

	template<typename... Ts> void operator() (const Ts&...);

	void operator<< (std::string);

	void setAxis(
		const std::string& = "x-axis",
		const std::string& = "y-axis",
		const std::string& = "z-axis"
	);

	void setSaveMode(const uint32_t, const uint32_t);

	void setWindowMode(const uint32_t, const uint32_t);

	void setOutput(const std::string&);

	void addLineStyle(
		const uint32_t,
		const std::string&,
		const uint32_t = 2,
		const uint32_t = 1,
		const uint32_t = 2,
		const float = 1.5f
	);

	void addGrid();

	template<typename T> void writeAndPlotData(
		const std::vector<PlotData2D<T>>&,
		const std::string& = "data.dat"
	);

	template<typename T> void writeData(
		const std::vector<PlotData2D<T>>&,
		const std::string& = "data.dat"
	);

	template<typename T> void plotData(
		const std::vector<PlotData2D<T>>&,
		const std::string&
	);

private:
	FILE* _gnuPlotPipe;
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

// --- PlotData2D ---

template<typename T> PlotData2D<T>::PlotData2D(
	const std::vector<std::pair<T, T>> points,
	const std::string& name
)
	:_points(points), _name(name)
{ }

template<typename T> PlotData2D<T>::PlotData2D(
	const std::vector<std::pair<T, T>> points,
	const uint32_t linestyle,
	const uint32_t index,
	const std::string& name
)
	: _points(points), _linestyle(linestyle), _index(index), _name(name)
{ }

// --- Gnuplot ---

template<typename... Ts> void Gnuplot::operator() (const Ts&... args)
{
	// send pack of elements that if needed will get converted to string command to gnuplot
	auto command = util::convert_to_string_v(args...);
	command += "\n";

	fprintf(_gnuPlotPipe, "%s\n", command.c_str());
	fflush(_gnuPlotPipe);
}

template<typename T> void PlotData2D<T>::setIndexAndLineStyle(
	const uint32_t index,
	const uint32_t linestyle
)
{
	_index = index;
	_linestyle = linestyle;
}

template<typename T> void Gnuplot::writeAndPlotData(
	const std::vector<PlotData2D<T>>& plots,
	const std::string& filePath
)
{
	writeData(plots, filePath);
	plotData(plots, filePath);
}

template<typename T> void Gnuplot::writeData(
	const std::vector<PlotData2D<T>>& plots,
	const std::string& filePath
)
{
	if (plots.size() > 0)
	{
		// write data to filePath
		std::ofstream outputFile(filePath);

		if (outputFile.fail()) {
			std::perror(filePath.c_str());
			return;
		}

		for (const auto& plot : plots)
		{
			outputFile << util::convert_to_string_v("#(index ", plot.getIndex(), ")\n");
			outputFile << "# X Y\n";
			for (const auto pointPair : plot.getData())
				outputFile << util::convert_to_string_v
				("  ", pointPair.first, " ", pointPair.second, "\n");

			outputFile << "\n\n";
		}
		outputFile.close();
	}
}

template<typename T> void Gnuplot::plotData(
	const std::vector<PlotData2D<T>>& plots,
	const std::string& filePath
)
{
	// create command and push it to gnuplot
	std::string command = "plot '" + filePath + "' ";
	for (const auto& plot : plots)
	{
		command += util::convert_to_string_v
		(
			"index ", plot.getIndex(),
			" t '", plot.getName(),
			"' with linespoints ls ", plot.getLinestyle(),
			", \'' "
		);
	}

	command.erase(command.size() - 5, 5); // remove the last ", \'' "

	operator()(command);
}

}

#endif // VOOL_GNP_H_INCLUDED