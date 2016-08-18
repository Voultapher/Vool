/*
* Vool - Unit tests for task_queue
*
* Copyright (C) 2016 by Lukas Bergdoll - www.lukas-bergdoll.net
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/
#pragma once

#include "TaskQueue.h"

#include <vector>
#include <string>
#include <stdexcept>
#include <random>

namespace vool
{

void test_TaskQueue()
{
	size_t testSize = static_cast<size_t>(1e4);

	auto simpleFunc([testSize](std::vector<int>& vec)
	{
		vec.clear();
		vec.resize(testSize);
		int startValue = 0;
		for (auto& element : vec)
			element = startValue++;
	});

	auto combinedFunc([testSize]
	(const std::vector<int>& vecA, const std::vector<int>& vecB, std::vector<int>& res)
	{
		auto itA = vecA.begin();
		auto itB = vecB.begin();
		auto itRes = res.begin();
		for (; itA != vecA.end() && itB != vecB.end(); ++itA, ++itB, ++itRes)
			*itRes = *itA + *itB;
	});

	// #1 basic
	{
		std::vector<int> vecA;
		std::vector<int> vecB;
		std::vector<int> res(testSize);

		{
			task_queue tq;

			// add 2 tasks which can execute in parallel
			auto conditionA = tq.add_task([&vecA, simpleFunc]() { simpleFunc(vecA); });
			auto conditionB = tq.add_task([&vecB, simpleFunc]() { simpleFunc(vecB); });

			// add a task that should only start as soon as the first 2 are finished
			tq.add_task
			(
				[&vecA, &vecB, &res, combinedFunc]()
				{ combinedFunc(vecA, vecB, res); }, // lambda body
				{ conditionA, conditionB } // requesites returned from adding task A and B
			);
		}

		if (res[testSize - 1] != ((testSize - 1) * 2)) // combinedFunc should fill res
			throw std::exception("taske_queue did not finish or missed a task");
	}

	// #2 optimal scaling
	{
		// optimal task should not be io bound
		auto optimalTask([testSize]()
		{ // non io bound test
			using val_t = volatile int;

			val_t val = {};
			for (val_t i = 0; i < testSize; ++i)
				val += static_cast<std::remove_volatile_t<val_t>>(sqrt(i));
		});

		task_queue tq;

		for (size_t i = 0; i < std::thread::hardware_concurrency(); ++i)
			tq.add_task([optimalTask]() { optimalTask(); });
	}

	// #3 wrong key
	{
		task_queue tq;

		async_t::key_t evil(1);
		tq.wait(async_t::public_key_t(evil));

		// this should not block
		// nonexisting prerequisites usually indicate an allready finished task
	}

	// #4 finished prereq
	{
		task_queue tq;

		int cat = 0;

		auto taskWait = [&cat]()
		{ std::this_thread::sleep_for(std::chrono::milliseconds(100)); cat = 7; };
		auto taskPrint = [&cat]() { int print = cat; static_cast<void>(print); };

		auto waitCond = tq.add_task(taskWait);
		tq.wait(waitCond);
		auto printCond = tq.add_task(taskPrint, { waitCond });
		tq.wait(printCond);

		// this should not block
	}

	// #5 many tasks
	{
		size_t taskCount = 300;

		task_queue tq;

		auto task = [](size_t i)
		{
			volatile int b = 38888394 % 856;
			static_cast<void>(b);
		};

		for (size_t i = 0; i < taskCount; ++i)
			tq.add_task([&task, i]() { task(i); });

		int check = 0;
		tq.wait(tq.add_task([&check]() { check = 10; }));

		if (check != 10)
			throw std::exception("the last task added was not finished");
	}

	// #6 lateTask
	{
		std::vector<int> v;

		task_queue tq;

		auto taskA = [&v, testSize]()
		{
			v.resize(testSize);
		};

		auto fail = true;
		auto taskB = [&v, &fail, testSize]()
		{
			fail = v.size() != testSize;
		};

		// add simple vector resize task
		auto condA = tq.add_task(taskA);
		tq.wait(condA);

		// add new task with condA as prerequisite
		tq.add_task(taskB, { condA });
		tq.wait_all();

		// wait_all should wait for one task and fail if taskA was improperly executed
		if (fail)
			throw std::exception("taskB was not properly executed");
	}

	// #7 taskception
	{
		task_queue tqB;

		size_t size = 1000;
		std::vector<int> v;
		async_t::public_key_t condition(async_t::key_t(0));

		std::promise<void> sync;

		auto resizeTask = [&v, &sync, size]() { sync.get_future().wait(); v.resize(size); };

		auto innerTask = [&tqB, &condition, &resizeTask]()
		{ condition = tqB.add_task(std::move(resizeTask)); };

		// a task is added to task_queue A, which itself adds task to task_queue B
		{
			task_queue tqA;
			auto condA = tqA.add_task(innerTask);
			tqA.wait(condA);
		}

		// now tqA should have finished innerTask and be empty
		// and tqB should have launched resizeTask and should wait for sync

		bool equalBefore = (v.size() == size);
		sync.set_value(); // signal resizeTask to continue
		tqB.wait(condition);
		bool equalAfter = (v.size() == size);

		if (equalBefore || !equalAfter)
			throw std::exception("some tasks were not properly executed");
	}

	// # 8 complex multilevel stability test
	auto heavyTest([](unsigned int seed)
	{
		using element_t = int;
		using sum_t = double;

		// create vector filled with vectors filled with random values
		std::vector<std::vector<element_t>> rndVecs;
		{
			size_t count = 64; // amount of vectors
			size_t size = 500; // amount of values per vector

			rndVecs.reserve(count);

			std::mt19937 generator;
			generator.seed(seed);

			std::uniform_int_distribution<element_t> distribution(-100, 1000);
			for (size_t i = 0; i < count; ++i)
			{
				std::vector<element_t> tmp;
				tmp.reserve(size);
				for (size_t b = 0; b < size; ++b)
					tmp.push_back(distribution(generator));

				rndVecs.push_back(std::move(tmp));
			}
		}

		// A: calculate the sum of n vectors of ints
		// B: do a calculation based on this new vector of sums
		// C: modify original vectors based on the result of B
		// D: rerun A and add its sums to the original sums
		// E: repeat B

		// used funcitons
		auto sum([](const std::vector<element_t>& v) -> sum_t
		{
			sum_t res = {};
			for (const auto element : v)
				res += element;
			return res;
		});

		auto groovle([](const std::vector<sum_t>& sums)-> element_t
		{
			element_t res = {};
			for (const auto sum : sums)
				res += static_cast<element_t>(sum / sqrt(sum)) % 2 == 0 ? 1 : 0;
			return res;
		});

		auto increaseGroov([](std::vector<element_t>& v, element_t groov)
		{
			for (auto& element : v)
				element += groov;
		});

		element_t groov;
		element_t groovSeq;

		// test task_queue
		{
			auto vecs = rndVecs; // create scope local copy of rndVecs

			std::vector<sum_t> sums;

			task_queue tq;

			// A
			sums.resize(vecs.size() * 2);
			auto sums_it = sums.begin();

			std::vector<async_t::public_key_t> conditionsA;
			for (const auto& vec : vecs)
				conditionsA.push_back(tq.add_task(
					[&vec, sum, it = sums_it++]() { *it = sum(vec); }));

			// B
			auto conditionB = tq.add_task(
				[&sums, &groov, groovle, size = vecs.size()]()
				{ 
					groov = groovle(std::vector<sum_t>(sums.begin(), sums.begin() + size));
				},
				conditionsA);

			// C
			std::vector<async_t::public_key_t> conditionsC;
			for (auto& vec : vecs)
				conditionsC.push_back(tq.add_task(([&vec, &groov, increaseGroov]()
			{ increaseGroov(vec, groov); }), { conditionB }));

			// D
			std::vector<async_t::public_key_t> conditionsD;
			for (const auto& vec : vecs)
				conditionsD.push_back(tq.add_task(
					[&vec, sum, it = sums_it++]() { *it = sum(vec); }, conditionsC));


			// E
			auto conditionE = tq.add_task([&sums, &groov, groovle]()
			{ groov = groovle(sums); }, conditionsD);

			tq.wait(conditionE);
		}

		// repeat all steps without task_queue
		{
			auto vecs = rndVecs; // create scope local copy of rndVecs

			std::vector<sum_t> sums;
			for (const auto& vec : vecs)
				sums.push_back(sum(vec));

			groovSeq = groovle(sums);

			for (auto& vec : vecs)
				increaseGroov(vec, groovSeq);

			for (const auto& vec : vecs)
				sums.push_back(sum(vec));

			groovSeq = groovle(sums);
		}

		if (groov != groovSeq)
			throw std::exception("something in complex test went wrong");
	});

	{
#ifdef NDEBUG
		size_t heavyRepeatCount = 500;
#else
		size_t heavyRepeatCount = 5;
#endif

		// repeat the complex test many times to catch rare multithreaded related bugs
		for (size_t i = 0; i < heavyRepeatCount; ++i)
			heavyTest(static_cast<unsigned int>(i) * 1445);
	}

}

}