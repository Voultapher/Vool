//#include "TaskQueue.h"

/*
* task_queue - Smart multithreading helper, designed for small overhead
*
* Copyright (c) 2016 Lukas Bergdoll - www.lukas-bergdoll.net
*
* This code is licensed under the AFL-3.0 license (http://opensource.org/licenses/AFL-3.0)
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


// --- async_task ---

async_task::async_task(
	const async_t::task_t& user_task,
	const async_t::prereq_t& prereqs
)
	:
	task(init_task(user_task)),
	_prerequisites(prereqs)
{
	active_flag.test_and_set(std::memory_order_acq_rel); // default to active
}

async_task::async_task(
	async_t::task_t&& user_task,
	const async_t::prereq_t& prereqs
)
	:
	task(init_task(std::forward<async_t::task_t>(user_task))),
	_prerequisites(prereqs)
{
	active_flag.test_and_set(std::memory_order_acq_rel); // default to active
}

inline async_t::task_t async_task::init_task(const async_t::task_t& user_task) noexcept
{
	async_t::task_t res([&user_task, &flag_ref = active_flag]() -> void
	{
		user_task();
		flag_ref.clear(std::memory_order_release);
	});
	return res;
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

const async_t::prereq_t& async_task::get_prerequisites() const
{
	return _prerequisites;
}


// --- task_queue ---

inline bool task_queue::task_active(async_task& task)
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

	for (const auto& prereq_key : (task_it->second.get_prerequisites()))
	{
		auto prereq_it = _tasks.find(prereq_key);

		if (prereq_it != _tasks.end())
		{
			relevant_tasks_keys.push_back(prereq_key);

			if (task_active(prereq_it->second))
				ready = false;
		}
		else
		{
			// should never happen without external manipulation
			throw std::exception(
				"A task from _tasks was removed although"
				"it still was a prerequisite!\n");
		}
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
		tasks_t::iterator task_it = _tasks.find(unstarted_key);

		if (task_it != _tasks.end())
		{
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
		else
		{
			// should never happen without external manipulation
			throw std::exception("An unstarted task was removed from _tasks\n");
		}
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
	// precreate resources for later reusability
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
	_unstarted_tasks_keys.push_back(_start_key);

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
	_unstarted_tasks_keys.push_back(_start_key);

	unlock();

	// return key to task structure, to be used by the user as prerequisite for later tasks
	// thread safe aslong _start_key is not being used in queue_loop
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

void task_queue::wait_all()
{
	finish_all_active_tasks();
}

}