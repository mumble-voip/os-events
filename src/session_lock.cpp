// SPDX-License-Identifier: BSD-3-Clause

#include <osevents/session_lock.hpp>

#ifdef OSEVENTS_USE_DBUS
#	include <osevents/details/dbus.hpp>
#	include <sdbus-c++/sdbus-c++.h>
#endif

#ifdef OSEVENTS_OS_WINDOWS
#	include <osevents/details/windows.hpp>
#	include <WtsApi32.h>
#endif

#include <atomic>
#include <cassert>
#include <memory>
#include <string>

namespace osevents {

struct SessionLockData {
	std::atomic< SessionLockState > state;
#ifdef OSEVENTS_USE_DBUS
	std::unique_ptr< sdbus::IProxy > screen_saver_proxy;
	std::shared_ptr< sdbus::IConnection > connection;
#endif
#ifdef OSEVENTS_OS_WINDOWS
	std::shared_ptr< details::WindowsEventLoop > event_loop;
#endif
};

SessionLock::SessionLock() : m_data(std::make_unique< SessionLockData >()) {
	SessionLockState initial_state = SessionLockState::Unlocked;

	auto callback = [this](bool activated) {
		SessionLockState state = activated ? SessionLockState::Locked : SessionLockState::Unlocked;

		SessionLockState old_state = m_data->state.exchange(state);

		if (state != old_state) {
			this->trigger(state);
		}
	};

#ifdef OSEVENTS_USE_DBUS
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
#ifdef OSEVENTS_OS_WINDOWS
	m_data->event_loop = details::windows_event_loop();
	assert(m_data->event_loop->is_running());

	if (!WTSRegisterSessionNotificationEx(WTS_CURRENT_SERVER, m_data->event_loop->message_window(),
										  NOTIFY_FOR_THIS_SESSION)) {
		throw std::runtime_error("Failed to subscribe to session notifications: " + std::to_string(GetLastError()));
	}

	details::WindowsEventLoop::EventHandler handler = [this, callback](HWND window, UINT event, WPARAM kind,
																	   LPARAM session) {
		assert(window == m_data->event_loop->message_window());
		assert(event == WM_WTSSESSION_CHANGE);
		(void) session;

		switch (kind) {
			case WTS_SESSION_LOCK:
				callback(true);
				break;
			case WTS_SESSION_UNLOCK:
				callback(false);
				break;
		}
	};

	m_data->event_loop->register_handler(WM_WTSSESSION_CHANGE, std::move(handler));
#endif

	m_data->state = initial_state;
}

SessionLock::~SessionLock() {
#ifdef OSEVENTS_OS_WINDOWS
	WTSUnRegisterSessionNotificationEx(WTS_CURRENT_SERVER, m_data->event_loop->message_window());
#endif
}

SessionLock::SessionLock(SessionLock &&) = default;

} // namespace osevents
