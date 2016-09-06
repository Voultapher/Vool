//#include "TaskQueue.h"

/*
* task_queue - Smart multithreading helper, designed for small overhead
*
* Copyright (C) 2016 by Lukas Bergdoll - www.lukas-bergdoll.net
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#include "TaskQueue.h"

#include <algorithm>

namespace vool
{

namespace async_t
{

public_key_t::public_key_t(const key_t& init) : _val(init) { }

template<typename T> public_key_t::public_key_t(const T& init)
{
	static_assert(std::is_same<T, async_t::key_t>::value,
		"Wrong public_key init type!");

	static_assert(!std::is_same<T, async_t::key_t>::value,
		"public_key_t overload constructor fail!");
}

const key_t& public_key_t::get() const { return _val; }

using prereq_t = std::vector<key_t>;
using public_prereq_t = std::vector<public_key_t>;
}

async_t::task_t async_task::init_task(const async_t::task_t& user_task) noexcept
{
	auto& flag_ref = flag;
	async_t::task_t res([&user_task, &flag_ref]() -> void
	{
		user_task();
		flag_ref.clear(std::memory_order_release);
	});
	return res;
}

async_t::task_t async_task::init_task(async_t::task_t&& user_task) noexcept
{
	auto& flag_ref = flag;
	async_t::task_t res([u_task = std::move(user_task), &flag_ref]() -> void
	{
		u_task();
		flag_ref.clear(std::memory_order_release);
	});
	return res;
}

async_task::async_task(
	const async_t::task_t& user_task,
	const async_t::prereq_t& prereqs
)
	:
	_prerequisites(prereqs),
	task(init_task(user_task))
{
	flag.test_and_set(std::memory_order_acq_rel); // default to active
}

async_task::async_task(
	async_t::task_t&& user_task,
	const async_t::prereq_t& prereqs
)
	:
	_prerequisites(prereqs),
	task(init_task(std::forward<async_t::task_t>(user_task)))
{
	flag.test_and_set(std::memory_order_acq_rel); // default to active
}

const async_t::prereq_t& async_task::get_prerequisites() const
{
	return _prerequisites;
}


void task_queue::queue_loop()
{
	// precreate resources for later reusability
	constexpr size_t min_reserve = 100;
	std::vector<decltype(_unstarted_tasks)::iterator> to_remove;
	std::vector<async_t::key_t> relevant_task_keys;
	std::vector<decltype(_tasks)::iterator> irrelevant_tasks;
	to_remove.reserve(min_reserve);
	relevant_task_keys.reserve(min_reserve);
	irrelevant_tasks.reserve(min_reserve);

	while (_active.test_and_set(std::memory_order_acq_rel))
	{
		to_remove.clear();
		relevant_task_keys.clear();
		irrelevant_tasks.clear();

		lock();

		// go through all unstarted tasks and launch those whose prerequisites are matched
		for (auto key_it = _unstarted_tasks.begin(); key_it != _unstarted_tasks.end(); ++key_it)
		{
			auto task_it = _tasks.find(*key_it);
			if (task_it != _tasks.end())
			{
				relevant_task_keys.push_back(*key_it); // unique remember all relevant tasks

				bool busy(false);
				for (const auto& prereq_key : task_it->second.get_prerequisites())
				{
					auto prereq_it = _tasks.find(prereq_key);
					if (prereq_it != _tasks.end())
					{
						// unique remember all relevant tasks
						relevant_task_keys.push_back(prereq_key);

						busy = prereq_it->second.flag.test_and_set(std::memory_order_acq_rel);
						if (busy)
							break;
						else
							prereq_it->second.flag.clear(std::memory_order_release);
					}
					else
					{
						// should never happen without external manipulation
						throw std::logic_error(
							"A task from _tasks was removed although"
							"it still was a prerequisite!\n");
					}
				}
				if (!busy)
				{
					// dispatch task
					task_it->second.future =
						std::async(std::launch::async, std::move(task_it->second.task));

					to_remove.push_back(key_it); // mark task for removal
				}
			}
			else
			{
				// should never happen without external manipulation
				throw std::logic_error("An unstarted task was removed from _tasks\n");
			}
		}

		// remove started tasks from _unstarted_tasks
		for (auto rem_it = to_remove.rbegin(); rem_it != to_remove.rend(); ++rem_it)
			_unstarted_tasks.erase(*rem_it); // *rem_it is a key_it

		// lower_bound requires a sorted container,
		// the usage of lower_bound should only start showing with heavy scaling

		if (relevant_task_keys.size() > 1)
		{
			std::sort(relevant_task_keys.begin(), relevant_task_keys.end());

			auto last = std::unique(relevant_task_keys.begin(), relevant_task_keys.end());
			relevant_task_keys.erase(last, relevant_task_keys.end());
		}

		// find all irrelevant tasks
		for (auto task_it = _tasks.begin(); task_it != _tasks.end(); ++task_it)
		{
			if (std::lower_bound(
				relevant_task_keys.begin(), relevant_task_keys.end(), task_it->first)
				== relevant_task_keys.end())
			{
				irrelevant_tasks.push_back(task_it);
			}
		}

		// remove all irrelevant tasks from _tasks
		for (auto task_it : irrelevant_tasks)
			_tasks.erase(task_it);

		unlock();
		std::this_thread::yield();
	}
}

async_t::prereq_t task_queue::remove_finished_prerequisites
(const async_t::public_prereq_t& prerequisites)
{
	async_t::prereq_t ret;
	ret.reserve(prerequisites.size());

	for (auto& prerequisite : prerequisites)
		if (_tasks.find(prerequisite.get()) != _tasks.end())
			ret.push_back(prerequisite.get());

	ret.shrink_to_fit();
	return ret;
}

void task_queue::finish_all_active_tasks()
{
	while (true)
	{ // wait until all tasks are finished
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

async_t::public_key_t task_queue::add_task(
	const async_t::task_t& task,
	async_t::public_prereq_t prerequisites
)
{
	lock();
	auto sanitzed = remove_finished_prerequisites(prerequisites);
	_tasks.emplace(std::piecewise_construct,
		std::make_tuple(_start_key),
		std::forward_as_tuple(task, sanitzed));
	_unstarted_tasks.push_back(_start_key);

	unlock();

	// return key to task structure, to be used by the user as prerequisite for later tasks
	// lock safe aslong _start_key is not being used in queue_loop
	return async_t::public_key_t(_start_key++);
}

async_t::public_key_t task_queue::add_task(
	async_t::task_t&& task,
	async_t::public_prereq_t prerequisites
)
{
	lock();
	auto sanitzed = remove_finished_prerequisites(prerequisites);
	_tasks.emplace(std::piecewise_construct,
		std::make_tuple(_start_key),
		std::forward_as_tuple(std::forward<async_t::task_t>(task), sanitzed));
	_unstarted_tasks.push_back(_start_key);

	unlock();

	// return key to task structure, to be used by the user as prerequisite for later tasks
	// lock safe aslong _start_key is not being used in queue_loop
	return async_t::public_key_t(_start_key++);
}

void task_queue::wait(const async_t::public_key_t& key)
{
	while (true)
	{
		lock();
		if (_tasks.find(key.get()) == _tasks.end())
			break; // task is finished and was deleted
		unlock();
		std::this_thread::yield();
	}
	unlock();
}

void task_queue::wait_all(const async_t::public_prereq_t keys)
{ // wait for all active tasks or a certain subset of tasks
	if (keys.size() == 0)
		finish_all_active_tasks();
	else
		for (const auto& key : keys)
			wait(key);
}

}