// SPDX-License-Identifier: BSD-3-Clause

#ifndef OSEVENTS_SESSION_LOCK_HPP_
#define OSEVENTS_SESSION_LOCK_HPP_

#include <osevents/event.hpp>

#include <memory>

namespace osevents {

enum class SessionLockState {
	Locked,
	Unlocked,
};

struct SessionLockData;

class SessionLock : public Event< void, SessionLockState > {
public:
	SessionLock();
	~SessionLock();

private:
	std::unique_ptr<SessionLockData> m_data;
};

} // namespace osevents

#endif
