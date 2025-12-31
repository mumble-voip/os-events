// SPDX-License-Identifier: BSD-3-Clause

#ifndef OSEVENTS_EVENT_HPP_
#define OSEVENTS_EVENT_HPP_

#include <cassert>
#include <functional>
#include <vector>

namespace osevents {

/**
 * Base class of all events in os-events. This class implements the managing and invoking of callback functions
 */
template< typename CallbackReturnValue, typename... CallbackArgs > class Event {
public:
	using Callback = std::function< CallbackReturnValue(CallbackArgs...) >;

	Event()          = default;
	virtual ~Event() = default;

	/**
	 * Registers the provided callback function with this event
	 *
	 * @param callback The callback to register
	 * @returns The callback's ID that can be used to deregister it at a later point
	 */
	std::size_t register_callback(Callback callback) {
		m_callbacks.emplace_back(std::move(callback));

		return m_callbacks.size();
	}

	/**
	 * Deregisters the callback with the given ID
	 *
	 * @param id The ID of the callback that shall be deregistered
	 *
	 * @note Once this function has been called with a given ID, the ID becomes invalid
	 * @note Calling this function with an invalid id is undefined behavior
	 */
	void deregister_callback(std::size_t id) {
		if (id >= m_callbacks.size()) {
			return;
		}

		if (m_callbacks.at(id)) {
			// Overwrite with an empty callback
			m_callbacks[id] = nullptr;
		}

		assert(!m_callbacks[id]);

		bool can_erase_to_end = true;
		for (std::size_t i = id + 1; i < m_callbacks.size(); ++i) {
			if (m_callbacks.at(i)) {
				can_erase_to_end = false;
				break;
			}
		}

		// If there are no valid (i.e. non-empty) functions following the one we just registered,
		// we can remove the entire range from our vector. Otherwise, we have to keep all entries in order to keep the
		// callback IDs (aka. indices) valid
		if (can_erase_to_end) {
			m_callbacks.erase(m_callbacks.begin() + id, m_callbacks.end());
		}
	}

	/**
	 * Deregisters all callbacks for this event
	 *
	 * @note This invalidates the IDs of all registered callbacks
	 */
	void clear_callbacks() { m_callbacks.clear(); }

protected:
	/**
	 * Sequentially calls all registered callbacks with the given arguments.
	 *
	 * @param args The arguments passed to the callbacks
	 *
	 * @note It is assumed that the arguments are left unmodified by the callbacks. Hence,
	 * they are simply passed-through without any protection about modification in case they
	 * are passed as non-const references.
	 */
	void trigger(CallbackArgs... args) const {
		for (const Callback &current : m_callbacks) {
			if (!current) {
				continue;
			}

			current(std::forward< CallbackArgs >(args)...);
		}
	}

private:
	std::vector< Callback > m_callbacks;
};

} // namespace osevents

#endif
