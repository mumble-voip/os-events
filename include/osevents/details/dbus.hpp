// SPDX-License-Identifier: BSD-3-Clause

#ifndef OSEVENTS_DETAILS_DBUS_HPP_
#define OSEVENTS_DETAILS_DBUS_HPP_

#include <memory>

namespace sdbus {
class IConnection;
};

namespace osevents::details {

/**
 * Provides access to a shared instance of a session DBus connection. All callers of this
 * function will obtain the same connection object, unless a previously created connection
 * has been destroyed in the meantime (i.e. all owners gave up their ownership). In that case,
 * a new connection will be established.
 *
 * @param start_event_loop Whether to ensure that the obtained connection has an active
 *        event loop running
 * @returns The desired connection object
 */
std::shared_ptr<sdbus::IConnection> session_dbus_connection(bool start_event_loop = true);

/**
 * Provides access to a shared instance of a system DBus connection. All callers of this
 * function will obtain the same connection object, unless a previously created connection
 * has been destroyed in the meantime (i.e. all owners gave up their ownership). In that case,
 * a new connection will be established.
 *
 * @param start_event_loop Whether to ensure that the obtained connection has an active
 *        event loop running
 * @returns The desired connection object
 */
std::shared_ptr<sdbus::IConnection> system_dbus_connection(bool start_event_loop = true);

}

#endif
