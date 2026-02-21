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

#include <algorithm>
#include <atomic>
#include <cassert>
#include <memory>
#include <ostream>
#include <string>
#include <vector>

#ifdef OSEVENTS_USE_DBUS
namespace osevents {

struct SessionInfo {
	std::string session_id;
	std::uint32_t user_id;
	std::string user_name;
	std::string seat_id;
	sdbus::ObjectPath session_path;
};

struct DBusSignalEndpoint {
	std::string service;
	std::string interface;
	std::string object;
	std::string signal;
};

} // namespace osevents

SDBUSCPP_REGISTER_STRUCT(osevents::SessionInfo, session_id, user_id, user_name, seat_id, session_path);
#endif

namespace osevents {

std::ostream &operator<<(std::ostream &stream, SessionLockState state) {
	switch (state) {
		case SessionLockState::Locked:
			stream << "Locked";
			return stream;
		case SessionLockState::Unlocked:
			stream << "Unlocked";
			return stream;
	}

	stream << "Invalid";
	return stream;
}

struct SessionLockData {
	std::atomic< SessionLockState > state;
#ifdef OSEVENTS_USE_DBUS
	std::vector< std::unique_ptr< sdbus::IProxy > > screen_saver_proxies;
	std::shared_ptr< sdbus::IConnection > session_connection;
	std::shared_ptr< sdbus::IConnection > system_connection;
#endif
#ifdef OSEVENTS_OS_WINDOWS
	std::shared_ptr< details::WindowsEventLoop > event_loop;
	std::size_t callback_id;
#endif
};


SessionLock::SessionLock() : m_data(std::make_unique< SessionLockData >()) {
	// Initialize with an invalid but well-defined state such that first change event is guaranteed to be recognized as
	// a transition into a new state
	m_data->state =
		static_cast< SessionLockState >(std::numeric_limits< std::underlying_type_t< SessionLockState > >::max());

	setup_callbacks();
}

SessionLock::~SessionLock() {
	if (m_data) {
#ifdef OSEVENTS_OS_WINDOWS
		WTSUnRegisterSessionNotificationEx(WTS_CURRENT_SERVER, m_data->event_loop->message_window());
#endif
		clear_callbacks();
	}
}

SessionLock::SessionLock(SessionLock &&other) {
	*this = std::move(other);
}

SessionLock &SessionLock::operator=(SessionLock &&other) {
	if (!other.m_data) {
		return *this;
	}

	// Deregister old callbacks
	other.clear_callbacks();

	m_data = std::move(other.m_data);

	// Re-register callbacks
	// This is to make sure that the callback functions are using the correct "this" pointer
	// which should now refer to this instead of other.
	setup_callbacks();

	return *this;
}

void SessionLock::setup_callbacks() {
	auto callback = [this](bool activated) {
		SessionLockState state = activated ? SessionLockState::Locked : SessionLockState::Unlocked;

		SessionLockState old_state = m_data->state.exchange(state);

		if (state != old_state) {
			this->trigger(state);
		}
	};

#ifdef OSEVENTS_USE_DBUS
	// Register some desktop environment (DE) specific DBus signals
	// Note: The freedesktop interface appears to be a KDE extension only (at this point)
	DBusSignalEndpoint endpoints[] = {
		{ .service   = "org.freedesktop.ScreenSaver",
		  .interface = "org.freedesktop.ScreenSaver",
		  .object    = "/org/freedesktop/ScreenSaver",
		  .signal    = "ActiveChanged" },
		{ .service   = "org.gnome.ScreenSaver",
		  .interface = "org.gnome.ScreenSaver",
		  .object    = "/org/gnome/ScreenSaver",
		  .signal    = "ActiveChanged" },
		{ .service   = "org.xfce.ScreenSaver",
		  .interface = "org.xfce.ScreenSaver",
		  .object    = "/org/xfce/ScreenSaver",
		  .signal    = "ActiveChanged" },
		{ .service   = "com.canonical.Unity",
		  .interface = "org.gnome.ScreenSaver",
		  .object    = "/org/gnome/ScreenSaver",
		  .signal    = "ActiveChanged" },
		{ .service   = "org.mate.ScreenSaver",
		  .interface = "org.mate.ScreenSaver",
		  .object    = "/org/mate/ScreenSaver",
		  .signal    = "ActiveChanged" },
	};
	for (const DBusSignalEndpoint &current : endpoints) {
		sdbus::ServiceName service(current.service);
		sdbus::InterfaceName interface(current.interface);

		sdbus::ObjectPath path(current.object);

		if (!m_data->session_connection) {
			m_data->session_connection = details::session_dbus_connection();
		}

		m_data->screen_saver_proxies.emplace_back(sdbus::createProxy(*m_data->session_connection, service, path));

		m_data->screen_saver_proxies.back()->uponSignal(current.signal).onInterface(interface).call(callback);
	}

	// Monitor changes to the systemd logind session property LockedHint
	if (!m_data->system_connection) {
		m_data->system_connection = details::system_dbus_connection();
	}
	sdbus::ServiceName service("org.freedesktop.login1");
	sdbus::ObjectPath path("/org/freedesktop/login1");

	// Determine active session
	auto proxy = sdbus::createProxy(*m_data->system_connection, service, path);
	std::vector< SessionInfo > sessions;
	proxy->callMethod("ListSessions").onInterface("org.freedesktop.login1.Manager").storeResultsTo(sessions);

	std::unique_ptr< sdbus::IProxy > active_session;
	for (const SessionInfo &current : sessions) {
		active_session = sdbus::createProxy(*m_data->system_connection, service, current.session_path);

		bool active = active_session->getProperty("Active").onInterface("org.freedesktop.login1.Session").get< bool >();
		if (active) {
			break;
		}

		active_session.reset();
	}

	if (active_session) {
		m_data->screen_saver_proxies.emplace_back(std::move(active_session));
		m_data->screen_saver_proxies.back()
			->uponSignal("PropertiesChanged")
			.onInterface("org.freedesktop.DBus.Properties")
			.call([callback](const sdbus::InterfaceName &interface_name,
							 const std::map< sdbus::PropertyName, sdbus::Variant > &changed_properties,
							 const std::vector< sdbus::PropertyName > &invalidated_properties) {
				(void) interface_name;
				(void) invalidated_properties;

				auto iter = changed_properties.find(sdbus::PropertyName("LockedHint"));
				if (iter == changed_properties.end()) {
					return;
				}

				bool locked = iter->second.get< bool >();

				callback(locked);
			});
	}

	// Watch for Unity-specific lockscreen service units
	// Note: they appear to be a bit laggy in terms of the timing in which they detect (un)lock events
	service = "org.freedesktop.systemd1";
	path = "/org/freedesktop/systemd1";
	m_data->screen_saver_proxies.emplace_back(sdbus::createProxy(*m_data->session_connection, service, path));
	m_data->screen_saver_proxies.back()->uponSignal("UnitNew").onInterface("org.freedesktop.systemd1.Manager").call(
		[callback] (const std::string &unit_name, const sdbus::ObjectPath &path) {
			if (unit_name == "unity-screen-locked.target") {
				callback(true);
			}
		}
	);
	m_data->screen_saver_proxies.back()->uponSignal("UnitRemoved").onInterface("org.freedesktop.systemd1.Manager").call(
		[callback] (const std::string &unit_name, const sdbus::ObjectPath &path) {
			if (unit_name == "unity-screen-locked.target") {
				callback(false);
			}
		}
	);

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

	m_data->callback_id = m_data->event_loop->register_handler(WM_WTSSESSION_CHANGE, std::move(handler));
#endif
}

void SessionLock::clear_callbacks() {
#ifdef OSEVENTS_USE_DBUS
	m_data->screen_saver_proxies.clear();
#endif
#ifdef OSEVENTS_OS_WINDOWS
	m_data->event_loop->deregister_handler(WM_WTSSESSION_CHANGE, m_data->callback_id);
#endif
}

} // namespace osevents
