#include <osevents/session_lock.hpp>

#include <cstdio>
#include <iostream>

int main() {
#if 0
	DBusConnection *dbus_conn = nullptr;
	DBusError dbus_err;

	// init error obj
	::dbus_error_init(&dbus_err);

	dbus_conn = ::dbus_bus_get(DBUS_BUS_SYSTEM, &dbus_err);

	std::cout << "Connected to D-Bus as \"" << ::dbus_bus_get_unique_name(dbus_conn) << "\"\n";

	DBusMessage *msg = nullptr;
	DBusMessage *reply = nullptr;

	// Create message to be sent
	msg = ::dbus_message_new_method_call("org.freedesktop.login1", "/org/freedesktop/login1/session", "org.freedesktop.DBus.Introspectable", "Introspect");

	// Actually send message / invoke method
	reply = ::dbus_connection_send_with_reply_and_block(dbus_conn, msg, DBUS_TIMEOUT_USE_DEFAULT, &dbus_err);

	const char *dbus_result = nullptr;

	// Parse response
	::dbus_message_get_args(reply, &dbus_err, DBUS_TYPE_STRING, &dbus_result, DBUS_TYPE_INVALID);
	std::cout << "Introspection result\n";
	std::cout << dbus_result << "\n";

	::dbus_message_unref(msg);
	::dbus_message_unref(reply);
	::dbus_connection_unref(dbus_conn);

		auto system_bus = sdbus::createSystemBusConnection();
	{
		sdbus::ServiceName service("org.freedesktop.login1");
		sdbus::ObjectPath path("/org/freedesktop/login1");

		auto proxy      = sdbus::createProxy(*system_bus, service, path);

		sdbus::InterfaceName interface("org.freedesktop.login1.Manager");
		sdbus::MethodName method("GetSession");
		// sdbus::InterfaceName interface("org.freedesktop.DBus.Introspectable");
		// sdbus::MethodName method("Introspect");
		auto call = proxy->createMethodCall(interface, method);

		call << "3";

		auto reply = proxy->callMethod(call);
		sdbus::ObjectPath result;
		reply >> result;
		std::cout << result << "\n";

		auto session = sdbus::createProxy(*system_bus, service, result);

		interface = "org.freedesktop.DBus.Introspectable";
		method    = "Introspect";
		call      = session->createMethodCall(interface, method);
		reply     = session->callMethod(call);
		std::string res;
		reply >> res;
		std::cout << res << "\n";

		// Seems like these signals are only relevant under weird circumstances
		// cmp. https://github.com/keepassxreboot/keepassxc/pull/910
		// Instead, we likely want to subscribe to the org.freedesktop.ScreenSaver's ActiveChanged signal
		// via the user session
		interface = "org.freedesktop.login1.session";
		sdbus::SignalName signal("Lock");
		session->registerSignalHandler(interface, signal, &lock_handler);
		signal = "Unlock";
		session->registerSignalHandler(interface, signal, &unlock_handler);
	}

		auto session_bus = sdbus::createSessionBusConnection();
	{
		sdbus::ServiceName service("org.freedesktop.ScreenSaver");
		sdbus::ObjectPath path("/org/freedesktop/ScreenSaver");
		sdbus::InterfaceName interface("org.freedesktop.ScreenSaver");
		sdbus::SignalName signal("ActiveChanged");

		auto proxy = sdbus::createProxy(*session_bus, service, path);

		proxy->registerSignalHandler(interface, signal, screensaver_handler);
	session_bus->enterEventLoop();
	}
#endif

	osevents::SessionLock lockWatcher;
	lockWatcher.register_callback([](osevents::SessionLockState state) {
		std::cout << "Session lock state changed to " << static_cast< int >(state) << "\n";
	});

	std::getchar();
}
