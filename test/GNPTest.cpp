/*
* Vool - Unit tests for GNP, as it will open a pipe to an external application, testability is limited
*
* Copyright (c) 2016 Lukas Bergdoll - www.lukas-bergdoll.net
*
* This code is licensed under the Apache License 2.0 (https://opensource.org/licenses/Apache-2.0)
*/

#include "AllTests.h"

#include <GNP.h>

#include <vector>
#include <string>
#include <exception>

namespace vool
{

namespace tests
{

void test_GNP()
{
	gnuplot::filepath_t gnuplot_filepath = "C:\\ProgramData\\gnuplot\\bin\\gnuplot";

	std::string cat = gnuplot_util::concatenate("cat ", 1, 2.f, " ", 3.3, " man");
	if (cat != "cat 12.000000 3.300000 man")
		throw std::exception("convert_to_string_v error");

	gnuplot gnp(gnuplot_filepath);

	gnp("set samples 10");
	gnp("set samples ", 150);

	gnp.name_axis("A", "B");
	gnp.set_terminal_window(1200, 500);
	gnp.add_linestyle(1, "#FF5A62", 2, 3, 5, 1.5f);
	gnp.add_grid();

	gnp("plot sin(x) ls 1"); // test plot, should open window and close it again

	gnp.set_terminal_png(1200, 500);
	gnp.set_png_filename("TestGraph");

	gnp("plot sin(x) ls 1"); // should save TestGraph.png in source directory

	gnp.set_png_filename("TestDataPlot");
	std::vector<std::pair<double, double>> dataPoints = { {1,4}, {3,2}, {4,7} };
	std::vector<plot_data_2D<double>> plot;
	plot.emplace_back(dataPoints, 0, "Test Points");
	gnp.write_and_plot(plot, "data\\GNPTestData.dat");
}

}

}