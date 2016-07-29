# Vool

C++ library

## Getting Started

Everything is header only. Every header can be compiled independently,
exceptions are TestSuit and all unit tests.

### Prerequisities

C++14 compliant compiler. Code is tested using MSVC and Clang. Should work cross-platform.

GNP requires gnuplot to be installed on the system and that its location is specified in GNPTest.h

### Installing

Download or copy header file. Include. Done.

Example:

```
#include "Vecmap.h"
```

## Running the tests

Easiest way is to download the entire project, compile it and take a look at main() in Source.cpp.

Make sure that the main folder is included, so that the unit tests can include the needed header files.

## Features

###Vool.h
A small collection of meta programming utilities and AritmeticStruct, a heterogeneous matrix.

```
using Ingredient = vool::ArithmeticStruct<float, int, double, char>;
auto cookie1 = Ingredient(3.4f, 42, 55.66, 'd');
auto cookie2 = Ingredient(4.f, 42, 72.3, 'z');
auto result = cookie1 * cookie2 + cookie1 - cookie2;
```

###Vecmap.h
A key value container built on top of std::vector
For an in depth explanation and benchmarks read this [blog post](http://www.lukas-bergdoll.net/blog/2016/1/31/big-o-pitfalls)

```
vool::vec_map<K, V> vecMap(static_cast<size_t>(1e6)); // construction and reserve
vecMap.insert(key, value); // key value insertion
V retValue = vecMap[key]; // value lookup using binary search
```

###GNP.h
A gnuplot pipe interface, built for convenience. Features include:
* Easy string concatenation
* Simple presets for live window or png output
* Ploting data using custom PlotData2D struct as data representation

```
const std::string gnuplotPath = "C:\\ProgramData\\gnuplot\\bin";

std::string converted = util::convert_to_string_v("cat ", 1, 2.f, " ", 3.3, " man");

Gnuplot gnp(gnuplotPath, false); // close after task termination

gnp.setAxis("A", "B"); // set axis names
gnp.setWindowMode(1200, 500); // set terminal to window
gnp.addLineStyle(1, "#FF5A62", 2, 3, 5, 1.5f); // add lineStyle
gnp.addGrid(); // add grid

std::vector<std::pair<double, double>> dataPoints = { {1,4}, {3,2}, {4,7} };
std::vector<PlotData2D<double>> plotData;
plotData.emplace_back(dataPoints, 1, 0, "Test Points");
gnp.plotData(plotData, "PlotResults\\PlotData\\GNPTestData.dat");
```

###TestSuit.h
Benchmarking tool using GNP to visualize its results.

```
size_t testSize = 1e4;

auto test = vool::createTest("vec build", [&simpleVec](const size_t size)
{
    size_t a = size % 2;
});

auto mainCategory = createTestCategory("Main", test);

vool::SuitConfiguration suitConfiguration; // setup Configuration
suitConfiguration.resultName = "Test Name"; // constomize Configuration

auto testSuit = vool::createTestSuit(suitConfiguration, mainCategory);
testSuit.runAllTests(0, testSize); // run tests in specified range
testSuit.renderResults(); // use GNP to outuput png showing the result
```

###TaskQueue.h
Smart multithreading helper, designed for small overhead.

```
task_queue tq;

// add 2 tasks which can execute in parallel
auto conditionA = tq.add_task([&vecA, simpleFunc]() { simpleFunc(vecA); });
auto conditionB = tq.add_task([&vecB, simpleFunc]() { simpleFunc(vecB); });

// add a task that should only start as soon as the first 2 are finished
tq.add_task
(
	[&vecA, &vecB, &res, combinedFunc]() // lambda carrying function
	{
        combinedFunc(vecA, vecB, res);
    },
	{ conditionA, conditionB } // requesites returned from adding task A and B
);
```

## Built With

* MSCV 15 update 3, or Clang 3.7

## Contributing

Fork and open a pull request

## Authors

* **Lukas Bergdoll** - [Blog](http://www.lukas-bergdoll.net/blog)

## License

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details
