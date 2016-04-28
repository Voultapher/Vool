// Unit test for GNP, as it will open a pipe to an external application, testability is limited
#pragma once

#include <vector>
#include <string>
#include <stdexcept>

#include <Vool\GNP.h>

namespace vool
{

static void test_GNP()
{
	const std::string gnuplotPath = "C:\\ProgramData\\gnuplot\\bin";

	std::string converted = util::convert_to_string_v("cat ", 1, 2.f, " ", 3.3, " man");
	if (converted != "cat 12.000000 3.300000 man")
		throw std::exception(); // convert_to_string_v error

	Gnuplot gnpOpen(gnuplotPath, true); // stay open until user closes gnuplot
	Gnuplot gnp(gnuplotPath, false); // close after task termination

	gnp << "set samples 10"; // operator<< call
	gnp("set samples ", 150); // operator() call

	gnp.setAxis("A", "B"); // set axis names
	gnp.setWindowMode(1200, 500); // set terminal to window
	gnp.addLineStyle(1, "#FF5A62", 2, 3, 5, 1.5f); // add lineStyle
	gnp.addGrid(); // add grid

	gnp << "plot sin(x) ls 1"; // test plot, should open window and close it again

	gnp.setSaveMode(1200, 500); // change terminal to output png
	gnp.setOutput("TestGraph"); // Name of .png that is output

	gnp << "plot sin(x) ls 1"; // should save TestPlot.png in source directory

	gnp.setOutput("TestDataPlot"); // Name of .png that is output
	std::vector<std::pair<double, double>> dataPoints = { {1,4}, {3,2}, {4,7} };
	std::vector<PlotData2D<double>> plotData;
	plotData.emplace_back(dataPoints, 1, 0, "Test Points");
	gnp.plotData(plotData, "PlotResults\\PlotData\\GNPTestData.dat");

	std::vector<PlotData2D<double>> plotDataEmpty;
	gnp.plotData(plotDataEmpty, "empty.dat"); // test empty, should create new file
}

}