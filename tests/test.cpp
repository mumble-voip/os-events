#include <osevents/session_lock.hpp>

#include <cstdio>
#include <iostream>

int main() {
	osevents::SessionLock lockWatcher;
	lockWatcher.register_callback([](osevents::SessionLockState state) {
		std::cout << "Session lock state changed to " << static_cast< int >(state) << "\n";
	});

	std::getchar();
}
