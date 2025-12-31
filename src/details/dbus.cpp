// SPDX-License-Identifier: BSD-3-Clause

#include <osevents/details/dbus.hpp>

#include <memory>

#include <sdbus-c++/sdbus-c++.h>

namespace osevents::details {

std::shared_ptr< sdbus::IConnection > session_dbus_connection(bool start_event_loop) {
	static std::weak_ptr< sdbus::IConnection > instance;

	std::shared_ptr< sdbus::IConnection > connection = instance.lock();

	if (!connection) {
		connection = std::shared_ptr< sdbus::IConnection >(sdbus::createSessionBusConnection());
		instance   = connection;
	}

	if (start_event_loop) {
		connection->enterEventLoopAsync();
	}

	return connection;
}

std::shared_ptr< sdbus::IConnection > system_dbus_connection(bool start_event_loop) {
	static std::weak_ptr< sdbus::IConnection > instance;

	std::shared_ptr< sdbus::IConnection > connection = instance.lock();

	if (!connection) {
		connection = std::shared_ptr< sdbus::IConnection >(sdbus::createSystemBusConnection());
		instance   = connection;
	}

	if (start_event_loop) {
		connection->enterEventLoopAsync();
	}

	return connection;
}

} // namespace osevents::details
