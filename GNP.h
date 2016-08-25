/*
* Vool - Gnuplot interface using popen/ _popen to pipe data to gnuplot
*
* Copyright (C) 2016 by Lukas Bergdoll - www.lukas-bergdoll.net
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/
#pragma once

#include <string>
#include <iostream>
#include <fstream>
#include <type_traits>

#ifdef _WIN32
#define pipe_open _popen
#define pipe_close _pclose
#else
#define pipe_open popen
#define pipe_close pclose
#endif

namespace vool
{

namespace util
{

	// automatic string conversion from variadic pack
	template<typename T> static typename std::enable_if<!(std::is_same<std::string, T>::value
		|| std::is_same<const char*, T>::value), std::string>::type convert(const T& arg)
	{ // sfinae overload if T is not an string or char*
		return std::move(std::to_string(arg));
	}

	template<typename T> static std::string convert(const std::string& arg) { return std::move(arg); }
	template<typename T> static std::string convert(const char* arg) { return std::move(arg); }

	template<typename... Ts> static std::string convert_to_string_v(const Ts&... args)
	{
		std::string command;
		auto wFunc = [&command](const auto& args)
		{
			if (!std::is_same<std::string, decltype(args)>::value)
				command += convert<decltype(args)>(args);
		};
		static_cast<void>
			(std::initializer_list<int> { 0, (wFunc(args), 0)... }); // order is important
		return std::move(command);
	}

}

template<typename T> struct PlotData2D
{
private:
	std::vector<std::pair<T, T>> _data;
	uint32_t _lineStyle;
	uint32_t _index;
	std::string _name;

public:
	PlotData2D(
		const std::vector<std::pair<T, T>> d,
		const uint32_t lS, const uint32_t id,
		const std::string& name
	)
		:_data(d), _lineStyle(lS), _index(id), _name(name) { }

	const auto& getData() const { return _data; }
	const auto getLineStyle() const { return _lineStyle; }
	const auto getIndex() const { return _index; }
	const auto& getName() const { return _name; }
};

class Gnuplot
{
private:
	FILE* _gnuPlotPipe;

	template<typename... Ts> void wrapper(Ts&&... args) { }

	template<typename T> static typename std::enable_if<!(std::is_same<std::string, T>::value
		|| std::is_same<const char*, T>::value), std::string>::type
		convert(const T& arg)
	{
		return std::move(std::to_string(arg));
	}

public:
	explicit Gnuplot(std::string gnuplotPath, bool persist = true)
	{
		gnuplotPath += persist ? "\\gnuplot -persist" : "\\gnuplot";
		_gnuPlotPipe = pipe_open(gnuplotPath.c_str(), "w");

		if (_gnuPlotPipe == NULL) // pipe_open itself failed
			throw std::exception("pipe_open failed");

		int ret_val = pipe_close(_gnuPlotPipe);
		if (ret_val != 0) // command failed
			throw std::exception("gnuplot filepath not found");

		_gnuPlotPipe = pipe_open(gnuplotPath.c_str(), "w");
	}

	Gnuplot(Gnuplot&& other) :
		_gnuPlotPipe(std::move(other._gnuPlotPipe))
	{
		other._gnuPlotPipe = nullptr; // indicate that it was moved
	}

	Gnuplot& operator=(Gnuplot&& other)
	{
		if (this != std::addressof(other))
		{
			_gnuPlotPipe = std::move(other._gnuPlotPipe);
			other._gnuPlotPipe = nullptr; // indicate that it was moved
		}
		return *this;
	}

	explicit Gnuplot(const Gnuplot& other) = delete; // no copy constructor

	Gnuplot& operator=(const Gnuplot& other) = delete; // no copy operator

	~Gnuplot() noexcept
	{
		if (_gnuPlotPipe != nullptr)
		{
			fprintf(_gnuPlotPipe, "exit\n");
			pipe_close(_gnuPlotPipe);
		}
	}

	template<typename... Ts> void operator() (const Ts&... args)
	{ // send pack of elements that if needed will get converted to string command to gnuplot
		auto command = util::convert_to_string_v(args...);
		command += "\n";

		fprintf(_gnuPlotPipe, "%s\n", command.c_str());
		fflush(_gnuPlotPipe);
	}

	void operator<< (std::string command)
	{ // send direct string command to gnuplot
		command += "\n";

		fprintf(_gnuPlotPipe, "%s\n", command.c_str());
		fflush(_gnuPlotPipe);
	}

	void setAxis(
		const std::string& xLabel = "x-axis",
		const std::string& yLabel = "y-axis",
		const std::string& zLabel = "z-axis"
	)
	{
		operator()("set xlabel \"" + xLabel + "\"");
		operator()("set ylabel \"" + yLabel + "\"");
		operator()("set zlabel \"" + zLabel + "\"");
	}

	void setSaveMode(const uint32_t horizontalRes, const uint32_t verticalRes)
	{
		operator()(
			"set terminal pngcairo enhanced font 'Verdana,10' background rgb '#FCFCFC' size ",
			horizontalRes, ", ", verticalRes);
	}

	void setWindowMode(const uint32_t horizontalRes, const uint32_t verticalRes)
	{
		operator()(
			"set terminal wxt enhanced font 'Verdana,10' background rgb '#FCFCFC' size ",
			horizontalRes, ", ", verticalRes);
	}

	void setOutput(const std::string& fileName)
	{ // subdirectorys dont work
		operator()("set output \"" + fileName + ".png\"");
	}

	void addLineStyle(
		const uint32_t index,
		const std::string& color,
		const uint32_t lineWidth = 2,
		const uint32_t lineType = 1,
		const uint32_t pointType = 2,
		const float pointSize = 1.5f
	)
	{
		operator()(
			"set style line ", index,
			" lc rgb \"" + color +
			"\" lw ", lineWidth,
			" dashtype ", lineType,
			" pt ", pointType,
			" ps ", pointSize);
	}

	void addGrid()
	{
		operator()("set style line 11 lc rgb '#4F4A4A' dashtype 1 lw 1");
		operator()("set border 3 back ls 11");
		operator()("set style line 12 lc rgb '#636161' dashtype 3 lw 1");
		operator()("set grid back ls 12");
	}

	template<typename T> void writeAndPlotData(
		const std::vector<PlotData2D<T>>& plots,
		const std::string& filePath = "data.dat"
	)
	{
		writeData(plots, filePath);
		plotData(plots, filePath);
	}

	template<typename T> void writeData(
		const std::vector<PlotData2D<T>>& plots,
		const std::string& filePath = "data.dat"
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

	template<typename T> void plotData(
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
				"' with linespoints ls ", plot.getLineStyle(),
				", \'' "
			);
		}

		command.erase(command.size() - 5, 5); // remove the last ", \'' "

		operator()(command);
	}
};

}