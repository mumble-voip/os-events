// SPDX-License-Identifier: BSD-3-Clause

#ifndef OSEVENTS_SESSION_LOCK_HPP_
#define OSEVENTS_SESSION_LOCK_HPP_

#include <osevents/event.hpp>
#include <osevents/export_macros.hpp>

#include <iosfwd>
#include <memory>

namespace osevents {

enum class OSEVENTS_EXPORT SessionLockState {
	Locked,
	Unlocked,
};

OSEVENTS_EXPORT std::ostream &operator<<(std::ostream &stream, SessionLockState state);

struct SessionLockData;

/**
 * Event for when the current session is locked or unlocked
 */
class OSEVENTS_EXPORT SessionLock : public Event< void, SessionLockState > {
public:
	SessionLock();
	~SessionLock();

	SessionLock(const SessionLock &) = delete;
	SessionLock(SessionLock &&);

	SessionLock &operator=(const SessionLock &) = delete;
	SessionLock &operator                       =(SessionLock &&);

private:
	std::unique_ptr< SessionLockData > m_data;

	void setup_callbacks();
	void clear_callbacks();
};

} // namespace osevents

#endif
