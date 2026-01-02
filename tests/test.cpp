#include <osevents/session_lock.hpp>

#include <cstdio>
#include <iostream>

#define OSEVENTS_SETUP_TEST_INSTANCES(Class, name) \
	Class name##_initial;                          \
	Class name = std::move(name##_initial);        \
	Class name##_additional;

int main() {
	OSEVENTS_SETUP_TEST_INSTANCES(osevents::SessionLock, lock_watcher);
	lock_watcher.register_callback(
		[](osevents::SessionLockState state) { std::cout << "Session lock state changed to " << state << "\n"; });

	std::cout << "All event handlers set up. Waiting for effects...\nPress any key to exit\n\n";

	std::getchar();
}

#undef OSEVENTS_SETUP_TEST_INSTANCES