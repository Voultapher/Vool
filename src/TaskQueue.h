/*
* Vool - Smart multithreading helper, designed for small overhead
*
* Copyright (c) 2016 Lukas Bergdoll - www.lukas-bergdoll.net
*
* This code is licensed under the AFL-3.0 license (http://opensource.org/licenses/AFL-3.0)
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

	struct public_key_t
	{
	public:
		explicit public_key_t(const key_t&);

		template<typename T> explicit public_key_t(const T&);

		const key_t& get() const;

	private:
		key_t _val;
	};

	using prereq_t = std::vector<key_t>;
	using public_prereq_t = std::vector<public_key_t>;
};

class async_task
{
public:
	std::atomic_flag active_flag;
	async_t::task_t task;

	// needed to prevent the std::future destructor from blocking
	std::future<void> future;

	explicit async_task(const async_t::task_t&, const async_t::prereq_t&);
	 
	explicit async_task(async_t::task_t&&, const async_t::prereq_t&);

	// no synchronization problems this way
	async_task(const async_task&) = delete;
	async_task(async_task&&) = delete;

	async_task& operator=(const async_task&) = delete;
	async_task& operator=(async_task&&) = delete;

	const async_t::prereq_t& get_prerequisites() const;

private:
	async_t::task_t init_task(const async_t::task_t&) noexcept;

	async_t::task_t init_task(async_t::task_t&&) noexcept;

	async_t::prereq_t _prerequisites;
};

class task_queue
{
public:
	using tasks_t = std::unordered_map<async_t::key_t, async_task>;
	using keys_t = std::vector<async_t::key_t>;

	explicit task_queue() noexcept;

	task_queue(const task_queue&) = delete;
	task_queue(task_queue&&) = delete;

	task_queue& operator=(const task_queue&) = delete;
	task_queue& operator=(task_queue&&) = delete;

	~task_queue() noexcept;

	async_t::public_key_t add_task(
		const async_t::task_t&,
		async_t::public_prereq_t = {}
	);

	async_t::public_key_t add_task(
		async_t::task_t&&,
		async_t::public_prereq_t = {}
	);

	void wait(const async_t::public_key_t&);

	void wait_all();

private:
	std::atomic_flag _sync; // lock synchronisation primitive
	std::atomic_flag _active; // queue loop lock synchronisation primitive

	async_t::key_t _start_key;
	keys_t _unstarted_tasks_keys;
	tasks_t _tasks;

	std::future<void> _queue_loop_future;

	bool task_active(async_task&);

	bool task_ready(const tasks_t::iterator, keys_t&);

	void launch_unstarted(keys_t&);

	void remove_finished_tasks(keys_t&);

	void queue_loop();

	async_t::prereq_t remove_finished_prerequisites(const async_t::public_prereq_t&);

	void finish_all_active_tasks();

	inline void lock() noexcept;

	inline void unlock() noexcept;
};

}

#endif // VOOL_TASKQUEUE_H_INCLUDED