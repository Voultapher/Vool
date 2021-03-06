/*
* Vool - gnuplot interface using popen to pipe data to gnuplot
*
* Copyright (c) 2016 Lukas Bergdoll - www.lukas-bergdoll.net
*
* This code is licensed under the Apache License 2.0 (https://opensource.org/licenses/Apache-2.0)
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

gnuplot::gnuplot(filepath_t filepath)
{
	//gnuplot_path += "\\gnuplot -persist";
	_gnuplot_pipe = PIPE_OPEN(filepath, "w");

	if (_gnuplot_pipe == NULL) // PIPE_OPEN itself failed
		throw std::exception("PIPE_OPEN failed");

	if (PIPE_CLOSE(_gnuplot_pipe) != 0) // command failed
		throw std::exception("gnuplot filepath not found");

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