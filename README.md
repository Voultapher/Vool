# Vool

C++ library

## Getting Started

A collection of c++ modules, that are build for cross-platform usage, small code footprint and performance

### Prerequisities

C++14 compliant compiler. Code is tested using MSVC and Clang. Should work cross-platform  
GNP requires gnuplot to be installed on the system and that its location is specified when constructing `vool::gnuplot`

### Installing

Include "include" folder into your project and `#include` wanted modules

Example:

```cpp
#include "Vecmap.h"
```

## Running the tests

Easiest way is to download the entire project, compile it and take a look at main() in test/Main.cpp

## Features

###Vecmap.h
A key value container built on top of std::vector  
For an in depth explanation and benchmarks read this [blog post](http://www.lukas-bergdoll.net/blog/2016/1/31/big-o-pitfalls)

```cpp
vool::vec_map<K, V> map;
map.insert(key, value);
V lookup = map[key]; // value lookup using binary search
```

###GNP.h
A gnuplot pipe interface, built for convenience. Features include:
* Easy string concatenation
* Simple presets for live window or png output
* Ploting data using `vool::plot_data_2D` struct as data representation

```cpp
vool::gnuplot::filepath_t gnuplot_filepath = "C:\\ProgramData\\gnuplot\\bin\\gnuplot";
vool::gnuplot gnp(gnuplot_filepath);

gnp("set samples 100");
gnp.name_axis("A", "B");
gnp.set_terminal_window(1200, 500);
gnp.add_linestyle(1, "#FF5A62", 2, 3, 5, 1.5f);
gnp.add_grid();

gnp("plot sin(x) ls 1"); // should open window and display plot of sin(x)

gnp.set_terminal_png(1200, 500);
gnp.set_png_filename("TestGraph");

gnp("plot sin(x) ls 1"); // should save plot of sin(x) as png file
```

###TestSuit.h
Benchmarking tool using GNP to visualize its results

```cpp
size_t size = 1000;

auto test_vec = vool::make_test("build vec",
    [](const size_t size)
    { std::vector<int> v(size); }
);

auto test_list = vool::make_test("build list",
    [](const size_t size)
    { std::list<int> l(size); }
);

auto category = vool::make_test_category("build", test_vec, test_list);

vool::suit_config config;
config.filename = "Tst_";

auto suit = vool::make_test_suit(config, category);
suit.perform_categorys(0, size);
suit.render_results();
```

###TaskQueue.h
Smart multithreading helper, designed for small overhead  
Internally using std::atomic_flag as synchronization primitive  
Tasks can be added from different threads

```cpp
{
vool::task_queue tq;

// add 2 tasks which can execute in parallel
auto condition_a = tq.add_task([&vecA] { func(vec_a); });
auto condition_b = tq.add_task([&vecB] { func(vec_b); });

// add a task that should only start as soon as the first 2 are finished
tq.add_task(
	[&vec_a, &vec_b, &res]() { combine(vec_a, vec_b, res) },
	{ condition_a, condition_b } // requesites returned from adding task A and B
);
}

// tq now out of scope
// task_queue destructor should block until all tasks are done
```

## Built With

* MSCV 15 update 3, or Clang 3.7

## Contributing

Fork and open a pull request

## Authors

* **Lukas Bergdoll** - [Blog](http://www.lukas-bergdoll.net/blog)

## License

This project is licensed under the Apache License 2.0 License - see the [LICENSE.md](LICENSE.md) file for details
