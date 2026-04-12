// SPDX-License-Identifier: BSD-3-Clause

#ifndef OSEVENTS_DETAILS_POLL_HPP_
#define OSEVENTS_DETAILS_POLL_HPP_

#include <chrono>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace osevents::details {

/**
 * Callable used to perform the polling
 */
using PollFunction = std::function< void() >;

/**
 * Collection of relevant data about a polling action
 */
struct PollData {
	std::size_t id;
	std::chrono::milliseconds interval;
	PollFunction poll;
};


/**
 * Central object that coordinates different polling actions such that they can all share the
 * same thread instead of each using their own.
 *
 * Usually, users shouldn't be creating their own instances of this class. Instead, use the poll_manager() function
 * to obtain a shared instance.
 */
class PollManager {
public:
	PollManager();
	~PollManager();

	/**
	 * Adds a new polling function to this manager
	 *
	 * @param id The ID of the polling event. If there already is a function with this ID, it will be overwritten
	 * @param interval The interval at which the polling shall be performed
	 * @param func The function performing the polling
	 *
	 * @see is_queued
	 */
	void enqueue(std::size_t id, std::chrono::milliseconds interval, PollFunction func);

	/**
	 * Removes a polling function
	 *
	 * @param id ID of the polling function that shall be removed
	 */
	void dequeue(std::size_t id);

	/**
	 * @returns Whether there is currently a polling function registered with the given ID
	 */
	bool is_queued(std::size_t id) const;

	/**
	 * @returns The interval at which the polling function with the given ID runs
	 */
	std::chrono::milliseconds interval(std::size_t id) const;
	/**
	 * Sets the polling interval for a polling function
	 *
	 * @param id ID of the polling function to change
	 * @param interval New polling interval for this function
	 */
	void set_interval(std::size_t id, std::chrono::milliseconds interval);

private:
	std::jthread m_thread;
	mutable std::mutex m_lock;
	std::vector< PollData > m_polls;
	std::condition_variable m_condition;

	void poll_loop();
};


/**
 * @returns A shared PollManager instance
 */
std::shared_ptr< PollManager > poll_manager();

} // namespace osevents::details

#endif
