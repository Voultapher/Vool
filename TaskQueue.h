/*
* task_queue - Smart multithreading helper, designed for small overhead
*
* Copyright (C) 2016 by Lukas Bergdoll - www.lukas-bergdoll.net
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#pragma once

#include <vector>
#include <unordered_map>
#include <set>
#include <iterator>
#include <functional>
#include <algorithm>

#include <thread>
#include <future>
#include <atomic>
#include <chrono>

namespace vool
{

namespace async_t
{
	using task_t = std::function<void()>;
	using key_t = uint64_t;

	struct public_key_t
	{
	private:
		key_t _val;

	public:
		explicit public_key_t(const key_t& init) : _val(init) { }

		template<typename T> explicit public_key_t(const T& init)
		{
			static_assert(std::is_same<T, async_t::key_t>::value,
				"Wrong public_key init type!");

			static_assert(!std::is_same<T, async_t::key_t>::value,
				"public_key_t overload constructor fail!");
		}

		const key_t& get() const { return _val; }
	};

	using prereq_t = std::vector<key_t>;
	using public_prereq_t = std::vector<public_key_t>;
};

struct async_task
{
private:

	inline async_t::task_t init_task(const async_t::task_t& user_task) noexcept
	{
		auto& flag_ref = flag;
		async_t::task_t res([&user_task, &flag_ref]() -> void
		{
			user_task();
			flag_ref.clear(std::memory_order_release);
		});
		return std::move(res);
	}

	inline async_t::task_t init_task(async_t::task_t&& user_task) noexcept
	{
		auto& flag_ref = flag;
		async_t::task_t res([u_task = std::move(user_task), &flag_ref]() -> void
		{
			u_task();
			flag_ref.clear(std::memory_order_release);
		});
		return std::move(res);
	}

	async_t::prereq_t _prerequisites;

public:
	std::atomic_flag flag;
	async_t::task_t task;

	// needed to prevent the std::future destructor from blocking
	std::future<void> future;

	explicit async_task(const async_t::task_t& user_task, const async_t::prereq_t& prereqs)
		: task(init_task(user_task)), _prerequisites(prereqs)
	{ // prerequisites constructor
		flag.test_and_set(std::memory_order_acq_rel); // default to active
	}

	explicit async_task(async_t::task_t&& user_task, const async_t::prereq_t& prereqs)
		: task(init_task(std::forward<async_t::task_t>(user_task))), _prerequisites(prereqs)
	{ // prerequisites constructor
		flag.test_and_set(std::memory_order_acq_rel); // default to active
	}

	const async_t::prereq_t& get_prerequisites() const
	{
		return _prerequisites;
	}
};

struct task_queue
{
private:
	std::atomic_flag _active; // queue loop lock synchronisation primitive
	std::atomic_flag _sync; // lock synchronisation primitive
	std::future<void> _queue_loop_future;

	async_t::key_t _start_key;
	std::vector<async_t::key_t> _unstarted_tasks;
	std::unordered_map<async_t::key_t, async_task> _tasks;

	void queue_loop()
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
							std::cout
								<< "A task from _tasks was removed although"
								<< " it still was a prerequisite!\n";
							throw std::logic_error("Invalid key!");
						}
					}
					if (!busy)
					{
						// dispatch task
						task_it->second.future = std::move(
							std::async(std::launch::async, std::move(task_it->second.task))
						);
						to_remove.push_back(key_it); // mark task for removal
					}
				}
				else
				{ // should never happen without external manipulation
					std::cout << "An unstarted task was removed from _tasks\n";
					throw std::logic_error("Invalid key!");
				}
			}

			// remove started tasks from _unstarted_tasks
			for (auto rem_it = to_remove.rbegin(); rem_it != to_remove.rend(); ++rem_it)
				_unstarted_tasks.erase(*rem_it); // *rem_it is a key_it

			// lower_bound requires a sorted container,
			// the usage of lower_bound should only start showing with heavy scaling

			if (relevant_task_keys.size() > 1)
			{ // use a set for sorting and making sure to have unique entries
				std::set<async_t::key_t> helper
					(relevant_task_keys.begin(), relevant_task_keys.end());

				relevant_task_keys.assign(helper.begin(), helper.end());
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

	async_t::prereq_t remove_finished_prerequisites
	(const async_t::public_prereq_t& prerequisites)
	{
		async_t::prereq_t ret;
		ret.reserve(prerequisites.size());

		for (auto& prerequisite : prerequisites)
			if (_tasks.find(prerequisite.get()) != _tasks.end())
				ret.push_back(prerequisite.get());

		ret.shrink_to_fit();
		return std::move(ret);
	}

	void finish_all_active_tasks()
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

	inline void lock() noexcept
	{
		while (_sync.test_and_set(std::memory_order_acq_rel));
	}

	inline void unlock() noexcept
	{
		_sync.clear(std::memory_order_release); // signal that the lock is free
	}

public:

	explicit task_queue() noexcept :
		_start_key(std::numeric_limits<async_t::key_t>::min())
	{
		_active.test_and_set(std::memory_order_acq_rel); // true state represents active state
		_sync.clear(std::memory_order_release); // true state represents active state

		// start the workthread
		std::function<void()> queue([this]() -> void { queue_loop(); });
		_queue_loop_future = std::async(std::launch::async, std::move(queue));
	}

	task_queue(const task_queue& other) = delete; // no copy constructor

	task_queue(task_queue&& other) = delete; // no move constructor

	task_queue& operator=(const task_queue& other) = delete; // no copy operator

	task_queue& operator=(task_queue&& other) = delete; // no move operator

	~task_queue() noexcept
	{
		finish_all_active_tasks();
		_active.clear(std::memory_order_release);

		_queue_loop_future.wait();
	}

	async_t::public_key_t add_task(
		const async_t::task_t& task,
		async_t::public_prereq_t prerequisites = {}
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

	async_t::public_key_t add_task(
		async_t::task_t&& task,
		async_t::public_prereq_t prerequisites = {}
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

	void wait(const async_t::public_key_t& key)
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

	void wait_all(const async_t::public_prereq_t keys = {})
	{ // wait for all active tasks or a certain subset of tasks
		if (keys.size() == 0)
			finish_all_active_tasks();
		else
			for (const auto& key : keys)
				wait(key);
	}
};

}