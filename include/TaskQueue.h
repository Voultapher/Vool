/*
* Vool - Smart multithreading helper, designed for small overhead
*
* Copyright (c) 2016 Lukas Bergdoll - www.lukas-bergdoll.net
*
* This code is licensed under the Apache License 2.0 (https://opensource.org/licenses/Apache-2.0)
*/

#ifndef VOOL_TASKQUEUE_H_INCLUDED
#define VOOL_TASKQUEUE_H_INCLUDED

#include <vector>
#include <unordered_map>
#include <future>

namespace vool
{

namespace async_t
{
	using task_t = std::function<void()>;
	using key_t = uint64_t;

	struct prereq
	{
	public:
		explicit prereq(const key_t&);

		template<typename T> explicit prereq(const T&);

		const key_t& key() const { return _val; }

	private:
		key_t _val;
	};
};

namespace task_queue_util
{
class async_task;
}

class task_queue
{
public:
	using tasks_t = std::unordered_map<async_t::key_t, task_queue_util::async_task>;
	using keys_t = std::vector<async_t::key_t>;

	explicit task_queue() noexcept;

	task_queue(const task_queue&) = delete;
	task_queue(task_queue&&) = delete;

	task_queue& operator=(const task_queue&) = delete;
	task_queue& operator=(task_queue&&) = delete;

	~task_queue() noexcept;

	async_t::prereq add_task(
		const async_t::task_t&
	);

	async_t::prereq add_task(
		const async_t::task_t&,
		std::vector<async_t::prereq>
	);

	async_t::prereq add_task(
		async_t::task_t&&
	);

	async_t::prereq add_task(
		async_t::task_t&&,
		std::vector<async_t::prereq>
	);

	void wait(const async_t::prereq&);

	void wait_all();

private:
	std::atomic_flag _sync; // lock synchronisation primitive
	std::atomic_flag _active; // queue loop lock synchronisation primitive

	async_t::key_t _start_key;
	keys_t _unstarted_tasks_keys;
	tasks_t _tasks;

	std::future<void> _queue_loop_future;

	bool task_active(task_queue_util::async_task&);

	bool task_ready(const tasks_t::iterator, keys_t&);

	void launch_unstarted(keys_t&);

	void remove_finished_tasks(keys_t&);

	void queue_loop();

	void remove_finished_prerequisites(std::vector<async_t::prereq>&);

	void finish_all_active_tasks();

	async_t::prereq emplace_task(
		async_t::task_t&&,
		std::vector<async_t::prereq>
	);
};

namespace task_queue_util
{
class atomic_lock
{
public:
	explicit atomic_lock(std::atomic_flag& flag) noexcept
		: _flag(flag)
	{
		// wait until the lock is free
		while (_flag.test_and_set(std::memory_order_acq_rel));
	}

	~atomic_lock() noexcept
	{
		// signal that the lock is free
		_flag.clear(std::memory_order_release);
	}

private:
	std::atomic_flag& _flag;
};

class async_task
{
public:
	std::atomic_flag active_flag;
	async_t::task_t task;

	// needed to prevent the std::future destructor from blocking
	std::future<void> future;

	explicit async_task(async_t::task_t&&, std::vector<async_t::prereq>);

	// no synchronization problems this way
	async_task(const async_task&) = delete;
	async_task(async_task&&) = delete;

	async_task& operator=(const async_task&) = delete;
	async_task& operator=(async_task&&) = delete;

	const std::vector<async_t::prereq>& prerequisites() const { return _prerequisites; }

private:
	async_t::task_t init_task(async_t::task_t&&) noexcept;

	std::vector<async_t::prereq> _prerequisites;
};
}

}

#endif // VOOL_TASKQUEUE_H_INCLUDED