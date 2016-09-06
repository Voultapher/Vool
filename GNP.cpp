/*
* Vool - Gnuplot interface using popen/ _popen to pipe data to gnuplot
*
* Copyright (C) 2016 by Lukas Bergdoll - www.lukas-bergdoll.net
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#include "GNP.h"

#ifdef _WIN32
#define PIPE_OPEN _popen
#define PIPE_CLOSE _pclose
#else
#define PIPE_OPEN popen
#define PIPE_CLOSE pclose
#endif

namespace vool
{

Gnuplot::Gnuplot(std::string gnuplotPath, bool persist)
{
	gnuplotPath += persist ? "\\gnuplot -persist" : "\\gnuplot";
	_gnuPlotPipe = PIPE_OPEN(gnuplotPath.c_str(), "w");

	if (_gnuPlotPipe == NULL) // PIPE_OPEN itself failed
		throw std::exception("PIPE_OPEN failed");

	int ret_val = PIPE_CLOSE(_gnuPlotPipe);
	if (ret_val != 0) // command failed
		throw std::exception("gnuplot filepath not found");

	_gnuPlotPipe = PIPE_OPEN(gnuplotPath.c_str(), "w");
}

Gnuplot::Gnuplot(Gnuplot&& other) :
	_gnuPlotPipe(std::move(other._gnuPlotPipe))
{
	other._gnuPlotPipe = nullptr; // indicate that it was moved
}

Gnuplot& Gnuplot::operator=(Gnuplot&& other)
{
	if (this != std::addressof(other))
	{
		_gnuPlotPipe = std::move(other._gnuPlotPipe);
		other._gnuPlotPipe = nullptr; // indicate that it was moved
	}
	return *this;
}

Gnuplot::~Gnuplot() noexcept
{
	if (_gnuPlotPipe != nullptr)
	{
		fprintf(_gnuPlotPipe, "exit\n");
		PIPE_CLOSE(_gnuPlotPipe);
	}
}

void Gnuplot::operator<< (std::string command)
{
	// send direct string command to gnuplot
	command += "\n";

	fprintf(_gnuPlotPipe, "%s\n", command.c_str());
	fflush(_gnuPlotPipe);
}

void Gnuplot::setAxis(
	const std::string& xLabel,
	const std::string& yLabel,
	const std::string& zLabel
)
{
	operator()("set xlabel \"" + xLabel + "\"");
	operator()("set ylabel \"" + yLabel + "\"");
	operator()("set zlabel \"" + zLabel + "\"");
}

void Gnuplot::setSaveMode(const uint32_t horizontalRes, const uint32_t verticalRes)
{
	operator()(
		"set terminal pngcairo enhanced font 'Verdana,10' background rgb '#FCFCFC' size ",
		horizontalRes, ", ", verticalRes);
}

void Gnuplot::setWindowMode(const uint32_t horizontalRes, const uint32_t verticalRes)
{
	operator()(
		"set terminal wxt enhanced font 'Verdana,10' background rgb '#FCFCFC' size ",
		horizontalRes, ", ", verticalRes);
}

void Gnuplot::setOutput(const std::string& fileName)
{ // subdirectorys dont work
	operator()("set output \"" + fileName + ".png\"");
}

void Gnuplot::addLineStyle(
	const uint32_t index,
	const std::string& color,
	const uint32_t lineWidth,
	const uint32_t lineType,
	const uint32_t pointType,
	const float pointSize
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

void Gnuplot::addGrid()
{
	operator()("set style line 11 lc rgb '#4F4A4A' dashtype 1 lw 1");
	operator()("set border 3 back ls 11");
	operator()("set style line 12 lc rgb '#636161' dashtype 3 lw 1");
	operator()("set grid back ls 12");
}

}

#undef PIPE_OPEN
#undef PIPE_CLOSE