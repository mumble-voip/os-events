// SPDX-License-Identifier: BSD-3-Clause

#include <osevents/session_lock.hpp>

#ifdef OSEVENTS_USE_DBUS
#	include <osevents/details/dbus.hpp>
#	include <sdbus-c++/sdbus-c++.h>
#endif

#include <atomic>
#include <memory>

namespace osevents {

struct SessionLockData {
	std::atomic< SessionLockState > state;
#ifdef OSEVENTS_USE_DBUS
	std::unique_ptr< sdbus::IProxy > screen_saver_proxy;
	std::shared_ptr< sdbus::IConnection > connection;
#endif
};

SessionLock::SessionLock() : m_data(std::make_unique< SessionLockData >()) {
	SessionLockState initial_state = SessionLockState::Unlocked;

#ifdef OSEVENTS_USE_DBUS
	auto callback = [this](bool activated) {
		SessionLockState state = activated ? SessionLockState::Locked : SessionLockState::Unlocked;

		SessionLockState old_state = m_data->state.exchange(state);

		if (state != old_state) {
			this->trigger(state);
		}
	};

	sdbus::ServiceName service("org.freedesktop.ScreenSaver");
	sdbus::ObjectPath path("/org/freedesktop/ScreenSaver");
	sdbus::InterfaceName interface("org.freedesktop.ScreenSaver");

	m_data->connection         = details::session_dbus_connection();
	m_data->screen_saver_proxy = sdbus::createProxy(*m_data->connection, service, path);

	bool is_locked = false;
	m_data->screen_saver_proxy->callMethod("GetActive").onInterface(interface).storeResultsTo(is_locked);

	initial_state = is_locked ? SessionLockState::Locked : SessionLockState::Unlocked;

	m_data->screen_saver_proxy->uponSignal("ActiveChanged").onInterface(interface).call(callback);
#endif

	m_data->state = initial_state;
}

SessionLock::~SessionLock() = default;

SessionLock::SessionLock(SessionLock &&) = default;

} // namespace osevents
