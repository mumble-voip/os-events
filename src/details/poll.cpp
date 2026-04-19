// SPDX-License-Identifier: BSD-3-Clause

#include <osevents/details/poll.hpp>

#include <algorithm>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <random>
#include <stdexcept>
#include <thread>
#include <vector>

namespace osevents::details {

bool operator==(const PollData &lhs, const PollData &rhs) {
	return lhs.id == rhs.id;
}

PollManager::PollManager() : m_thread(&PollManager::poll_loop, this) {
}

PollManager::~PollManager() {
	m_stop_requested.store(true);
	m_condition.notify_all();
	m_thread.join();
}

void PollManager::enqueue(std::size_t id, std::chrono::milliseconds interval, PollFunction func) {
	// Notify before locking?
	std::unique_lock lock(m_lock);

	PollData data{ .id = id, .interval = std::move(interval), .poll = std::move(func) };

	auto it = std::ranges::find(m_polls, data);

	if (it != m_polls.end()) {
		// Overwrite
		*it = std::move(data);
	} else {
		m_polls.push_back(std::move(data));
	}

	m_condition.notify_all();
}

void PollManager::dequeue(std::size_t id) {
	std::unique_lock lock(m_lock);

	auto it = std::ranges::find(m_polls, id, &PollData::id);

	if (it == m_polls.end()) {
		return;
	}

	m_polls.erase(it);
}

bool PollManager::is_queued(std::size_t id) const {
	std::unique_lock lock(m_lock);

	auto it = std::ranges::find(m_polls, id, &PollData::id);

	return it != m_polls.end();
}

std::size_t PollManager::create_id() const {
	std::unique_lock lock(m_lock);

	std::random_device dev;
	std::mt19937_64 rng(dev());

	std::uniform_int_distribution< std::mt19937_64::result_type > dist(
		0, std::numeric_limits< std::mt19937_64::result_type >::max());

	std::size_t id = dist(rng);
	while (std::ranges::find(m_polls, id, &PollData::id) != m_polls.end()) {
		id = dist(rng);
	}

	return id;
}

std::chrono::milliseconds PollManager::interval(std::size_t id) const {
	std::unique_lock lock(m_lock);

	auto it = std::ranges::find(m_polls, id, &PollData::id);

	if (it == m_polls.end()) {
		throw std::invalid_argument("Attempting to access poll data with unknown ID");
	}

	return it->interval;
}

void PollManager::set_interval(std::size_t id, std::chrono::milliseconds interval) {
	std::unique_lock lock(m_lock);

	auto it = std::ranges::find(m_polls, id, &PollData::id);

	if (it == m_polls.end()) {
		return;
	}

	it->interval = std::move(interval);
}

void PollManager::poll_loop() {
	std::unique_lock lock(m_lock);

	std::vector< std::size_t > multiples;

	std::chrono::steady_clock::time_point wakeup_time = std::chrono::steady_clock::now();
	while (!m_stop_requested.load()) {
		if (m_polls.empty()) {
			// Wait until we receive data
			m_condition.wait(lock);
			continue;
		}

		// Determine minimum wait interval
		const std::chrono::milliseconds wait_interval =
			std::ranges::min(m_polls, std::less<>{}, &PollData::interval).interval;

		multiples.resize(m_polls.size());
		std::size_t max_multiple = 0;

		// Convert all intervals into multiples of wait_interval (erring on the side of polling earlier rather than
		// later due to the way we round)
		for (std::size_t i = 0; i < m_polls.size(); ++i) {
			const PollData &data = m_polls[i];
			std::size_t multiple = data.interval / wait_interval;

			multiples[i] = std::max(multiple, static_cast< std::size_t >(1));
			max_multiple = std::max(multiples[i], max_multiple);
		}

		// Outer loop whose only purpose is to re-run the inner for loop
		bool keep_going = true;
		while (keep_going) {
			wakeup_time = std::chrono::steady_clock::now();

			// Perform max_multiple iterations after which every poll has been executed at least once
			// We restart the loop rather than letting it run forever to avoid issues due to overflowing
			// our iteration counter.
			for (std::size_t iteration = 1; iteration <= max_multiple; ++iteration) {
				wakeup_time += wait_interval;

				std::cv_status stat = m_condition.wait_until(lock, wakeup_time);

				if (stat != std::cv_status::timeout) {
					// We woke up because the condition variable has been notified (which means data has changed)
					// -> exit this loop and the keep_going while loop to perform a fresh initialization of multiples
					// with the new data.
					// (this can also happen due to a spurious wakeup, which we will simply treat as if data had
					// changed)
					keep_going = false;
					break;
				}

				for (std::size_t i = 0; i < m_polls.size(); ++i) {
					if ((iteration % multiples[i]) != 0) {
						continue;
					}

					m_polls[i].poll();
				}
			}
		}
	}
}


std::shared_ptr< PollManager > poll_manager() {
	static std::weak_ptr< PollManager > instance;

	std::shared_ptr< PollManager > manager = instance.lock();

	if (!manager) {
		manager  = std::make_shared< PollManager >();
		instance = manager;
	}

	return manager;
}

} // namespace osevents::details
