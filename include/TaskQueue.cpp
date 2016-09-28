/*
* Vool - Smart multithreading helper, designed for small overhead
*
* Copyright (c) 2016 Lukas Bergdoll - www.lukas-bergdoll.net
*
* This code is licensed under the Apache License 2.0 (https://opensource.org/licenses/Apache-2.0)
*/

#include "TaskQueue.h"

#include <algorithm>
#include <cassert>

namespace vool
{

namespace async_t
{

// --- prereq ---

prereq::prereq(const key_t& init) : _val(init) { }

template<typename T> prereq::prereq(const T& init)
{
	static_assert(std::is_same<T, async_t::key_t>::value,
		"Wrong public_key init type!");

	static_assert(!std::is_same<T, async_t::key_t>::value,
		"prereq overload constructor fail!");
}

}

// --- task_queue ---

inline bool task_queue::task_active(task_queue_util::async_task& task)
{
	if (task.active_flag.test_and_set(std::memory_order_acq_rel))
		return true;
	else
		task.active_flag.clear(std::memory_order_release);
	return false;
}


bool task_queue::task_ready(
	const tasks_t::iterator task_it,
	keys_t& relevant_tasks_keys
)
{
	bool ready = true;

	for (const auto& prerequisite : (task_it->second.prerequisites()))
	{
		auto task_it = _tasks.find(prerequisite.key());
		assert(task_it != _tasks.end() &&
			"A task from _tasks was removed although it still was a prerequisite!");

		relevant_tasks_keys.push_back(prerequisite.key());

		if (task_active(task_it->second))
			ready = false;
	}

	return ready;
}

void task_queue::launch_unstarted(
	keys_t& relevant_tasks_keys
)
{
	keys_t still_unstarted_tasks_keys;
	for (const auto unstarted_key : _unstarted_tasks_keys)
	{
		auto task_it = _tasks.find(unstarted_key);
		assert(task_it != _tasks.end() && "An unstarted task was removed from _tasks");

		relevant_tasks_keys.push_back(unstarted_key);

		if (task_ready(task_it, relevant_tasks_keys))
		{
			// dispatch task
			task_it->second.future =
				std::async(std::launch::async, std::move(task_it->second.task));
		}
		else
			still_unstarted_tasks_keys.push_back(unstarted_key);
	}

	_unstarted_tasks_keys = std::move(still_unstarted_tasks_keys);
}

void task_queue::remove_finished_tasks(
	keys_t& keys
)
{
	std::sort(keys.begin(), keys.end());
	auto last = std::unique(keys.begin(), keys.end());
	keys.erase(last, keys.end());

	keys_t irrelevant_keys;
	for (auto& task : _tasks)
		if (std::binary_search(keys.begin(), keys.end(), task.first) == false)
			if (task_active(task.second) == false)
				irrelevant_keys.push_back(task.first);


	for (const auto key : irrelevant_keys)
		_tasks.erase(key);
}


void task_queue::queue_loop()
{
	constexpr size_t min_reserve = 100;

	keys_t relevant_tasks_keys;

	relevant_tasks_keys.reserve(min_reserve);

	while (_active.test_and_set(std::memory_order_acq_rel))
	{
		relevant_tasks_keys.clear();

		lock();

		launch_unstarted(relevant_tasks_keys);

		remove_finished_tasks(relevant_tasks_keys);

		unlock();
		std::this_thread::yield();
	}
}

void task_queue::remove_finished_prerequisites
(std::vector<async_t::prereq>& prerequisites)
{
	
	prerequisites.erase(
		std::remove_if(prerequisites.begin(), prerequisites.end(),
			[&tasks = _tasks](const auto prerequisite) -> bool
			{ return tasks.count(prerequisite.key()) == 0; }
		),
		prerequisites.end()
	);
}

async_t::prereq task_queue::emplace_task(
	async_t::task_t&& task,
	std::vector<async_t::prereq> prerequisites
)
{
	lock();

	if (prerequisites.size() > 0)
		remove_finished_prerequisites(prerequisites);

	_tasks.emplace(std::piecewise_construct,
		std::make_tuple(_start_key),
		std::forward_as_tuple(
			std::forward<async_t::task_t>(task),
			prerequisites
		)
	);
	_unstarted_tasks_keys.push_back(_start_key);

	async_t::prereq prerequisite(_start_key);
	++_start_key;

	unlock();

	return prerequisite;
}

void task_queue::finish_all_active_tasks()
{
	while (true)
	{
		// wait until all tasks are finished
		lock();
		if (_tasks.size() == 0)
			break;
		unlock();
		std::this_thread::yield();
	}
	unlock();
}

inline void task_queue::lock() noexcept
{
	while (_sync.test_and_set(std::memory_order_acq_rel));
}

inline void task_queue::unlock() noexcept
{
	_sync.clear(std::memory_order_release); // signal that the lock is free
}


task_queue::task_queue() noexcept :
	_start_key(std::numeric_limits<async_t::key_t>::min())
{
	_active.test_and_set(std::memory_order_acq_rel); // true state represents active state
	_sync.clear(std::memory_order_release); // true state represents active state

											// start the workthread
	std::function<void()> queue([this]() -> void { queue_loop(); });
	_queue_loop_future = std::async(std::launch::async, std::move(queue));
}

task_queue::~task_queue() noexcept
{
	finish_all_active_tasks();
	_active.clear(std::memory_order_release);

	_queue_loop_future.wait();
}

async_t::prereq task_queue::add_task(
	const async_t::task_t& task
)
{
	return emplace_task(async_t::task_t(task), {});
}

async_t::prereq task_queue::add_task(
	const async_t::task_t& task,
	std::vector<async_t::prereq> prerequisites
)
{
	return emplace_task(async_t::task_t(task), prerequisites);
}

async_t::prereq task_queue::add_task(
	async_t::task_t&& task
)
{
	return emplace_task(std::forward<async_t::task_t>(task), {});
}

async_t::prereq task_queue::add_task(
	async_t::task_t&& task,
	std::vector<async_t::prereq> prerequisites
)
{
	return emplace_task(std::forward<async_t::task_t>(task), prerequisites);
}

void task_queue::wait(const async_t::prereq& prerequisite)
{
	while (true)
	{
		lock();
		if (_tasks.find(prerequisite.key()) == _tasks.end())
			break; // task is finished and was deleted
		unlock();
		std::this_thread::yield();
	}
	unlock();
}

void task_queue::wait_all()
{
	finish_all_active_tasks();
}

namespace task_queue_util
{

// --- async_task ---

async_task::async_task(
	async_t::task_t&& user_task,
	std::vector<async_t::prereq> prereqs
)
	:
	task(init_task(std::forward<async_t::task_t>(user_task))),
	_prerequisites(prereqs)
{
	active_flag.test_and_set(std::memory_order_acq_rel); // default to active
}

inline async_t::task_t async_task::init_task(async_t::task_t&& user_task) noexcept
{
	async_t::task_t res([u_task = std::move(user_task), &flag_ref = active_flag]() -> void
	{
		u_task();
		flag_ref.clear(std::memory_order_release);
	});
	return res;
}

}


}