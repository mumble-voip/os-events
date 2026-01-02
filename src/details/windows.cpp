// SPDX-License-Identifier: BSD-3-Clause

#include <osevents/details/windows.hpp>

#include <cassert>
#include <chrono>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

namespace osevents::details {

std::weak_ptr< WindowsEventLoop > win_event_loop;

// cmp. https://learn.microsoft.com/en-us/windows/win32/api/winuser/nc-winuser-wndproc
LRESULT WINAPI window_callback(HWND hwnd, UINT event, WPARAM wParam, LPARAM lParam) {
	std::shared_ptr< WindowsEventLoop > loop = win_event_loop.lock();

	if (loop) {
		loop->handle(hwnd, event, wParam, lParam);
	}

	return DefWindowProc(hwnd, event, wParam, lParam);
}

WindowsEventLoop::WindowsEventLoop() {
	LPWNDCLASSEX dummy = {};
	(void) dummy;
	if (!GetClassInfoEx(own_windows_module_handle(), message_window_class.data(), dummy)) {
		std::cout << "Class not registered code: " << GetLastError() << std::endl;

		// Register the window class we intend to use for the purpose of this event loop
		WNDCLASSEX wx    = {};
		wx.cbSize        = sizeof(WNDCLASSEX);
		wx.lpfnWndProc   = window_callback; // function which will handle messages
		wx.hInstance     = own_windows_module_handle();
		wx.lpszClassName = message_window_class.data();

		if (!RegisterClassEx(&wx)) {
			throw std::runtime_error("Failed at registering Windows message window class: "
									 + std::to_string(GetLastError()));
		}
	}

	m_thread = std::jthread(std::bind_front(&WindowsEventLoop::event_loop, this));
}

WindowsEventLoop::~WindowsEventLoop() {
	if (m_msg_window) {
		DestroyWindow(m_msg_window);
		m_thread.request_stop();
	}
}

bool WindowsEventLoop::is_running() const {
	return m_running;
}

std::size_t WindowsEventLoop::register_handler(UINT event, EventHandler handler) {
	std::unique_lock lock(m_handler_mutex);

	m_handlers[event].emplace_back(std::move(handler));

	return m_handlers[event].size() - 1;
}

void WindowsEventLoop::deregister_handler(UINT event, std::size_t handler_id) {
	std::unique_lock lock(m_handler_mutex);

	if (!m_handlers.contains(event)) {
		return;
	}

	std::vector< EventHandler > &handler_list = m_handlers.at(event);

	if (handler_id >= handler_list.size()) {
		return;
	}

	if (handler_list[handler_id]) {
		handler_list[handler_id] = nullptr;
	}
	assert(!handler_list[handler_id]);

	bool delete_to_end = true;
	for (std::size_t i = handler_id + 1; handler_list.size(); ++i) {
		if (handler_list[i]) {
			delete_to_end = false;
			break;
		}
	}

	if (delete_to_end) {
		handler_list.erase(handler_list.begin() + handler_id, handler_list.end());

		if (handler_list.empty()) {
			m_handlers.erase(handler_id);
		}
	}
}

void WindowsEventLoop::clear_handlers(UINT event) {
	std::unique_lock lock(m_handler_mutex);

	m_handlers.erase(event);
}

void WindowsEventLoop::clear_all_handlers() {
	std::unique_lock lock(m_handler_mutex);

	m_handlers.clear();
}

void WindowsEventLoop::handle(HWND window, UINT event, WPARAM wparam, LPARAM lparam) {
	std::unique_lock lock(m_handler_mutex);

	if (!m_handlers.contains(event)) {
		return;
	}

	for (const EventHandler &handler : m_handlers.at(event)) {
		if (!handler) {
			continue;
		}

		handler(window, event, wparam, lparam);
	}
}

HWND WindowsEventLoop::message_window() const {
	assert(m_msg_window);
	return m_msg_window;
}

class RunningTracker {
public:
	RunningTracker(std::atomic< bool > &flag) : m_flag(flag) { m_flag = true; }

	~RunningTracker() { m_flag = false; }

private:
	std::atomic< bool > &m_flag;
};

void WindowsEventLoop::event_loop(std::stop_token token) {
	assert(!m_msg_window);

	m_msg_window = CreateWindowEx(0, message_window_class.data(), message_window_name.data(), 0, 0, 0, 0, 0,
								  HWND_MESSAGE, NULL, own_windows_module_handle(), NULL);

	if (!m_msg_window) {
		throw std::runtime_error("Failed at creating the Windows message window: " + std::to_string(GetLastError()));
	}

	RunningTracker tracker(m_running);

	// The actual event loop
	// see https://learn.microsoft.com/en-us/windows/win32/winmsg/about-messages-and-message-queues#message-loop
	MSG msg;
	BOOL bRet;
	while ((bRet = GetMessage(&msg, m_msg_window, 0, 0)) != 0) {
		if (bRet == -1) {
			// error -> exit event loop
		} else {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		if (token.stop_requested()) {
			break;
		}
	}
}


std::shared_ptr< WindowsEventLoop > windows_event_loop() {
	std::shared_ptr< WindowsEventLoop > loop = win_event_loop.lock();

	if (!loop) {
		loop           = std::make_shared< WindowsEventLoop >();
		win_event_loop = loop;

		std::size_t attempts = 0;
		while (!loop->is_running()) {
			std::this_thread::sleep_for(std::chrono::microseconds(5 * (attempts * 10 + 1)));

			if (attempts > 100) {
				throw std::runtime_error("Waiting on event loop to be started timed out - this likely indicates some "
										 "error inside the event loop");
			}

			attempts++;
		}
	}

	return loop;
}

// see https://devblogs.microsoft.com/oldnewthing/20041025-00/?p=37483
// and https://stackoverflow.com/a/78906765
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
HMODULE own_windows_module_handle() {
	return ((HMODULE) &__ImageBase);
}

} // namespace osevents::details