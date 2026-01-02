// SPDX-License-Identifier: BSD-3-Clause

#ifndef OSEVENTS_DETAILS_WINDOWS_HPP_
#define OSEVENTS_DETAILS_WINDOWS_HPP_

#include <atomic>
#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string_view>
#include <thread>
#include <vector>

#define STRICT
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#undef NOMINMAX
#undef WIN32_LEAN_AND_MEAN
#undef STRICT


namespace osevents::details {

class WindowsEventLoop {
public:
	static constexpr std::string_view message_window_class = "osevents_message_window_class";
	static constexpr std::string_view message_window_name  = "osevents_message_window";

	using EventHandler = std::function< void(HWND, UINT, WPARAM, LPARAM) >;

	WindowsEventLoop();
	~WindowsEventLoop();

	WindowsEventLoop(const WindowsEventLoop &) = delete;
	WindowsEventLoop(WindowsEventLoop &&)      = delete;

	bool is_running() const;

	std::size_t register_handler(UINT event, EventHandler handler);
	void deregister_handler(UINT event, std::size_t handler_id);

	void clear_handlers(UINT event);
	void clear_all_handlers();

	void handle(HWND window, UINT event, WPARAM wparam, LPARAM lparam);

	HWND message_window() const;

private:
	std::jthread m_thread;
	std::atomic< bool > m_running = false;
	HWND m_msg_window             = nullptr;
	std::map< UINT, std::vector< EventHandler > > m_handlers;
	std::mutex m_handler_mutex;

	void event_loop(std::stop_token token);
};


std::shared_ptr< WindowsEventLoop >
	windows_event_loop(std::chrono::milliseconds timeout = std::chrono::milliseconds(100));

HMODULE own_windows_module_handle();

} // namespace osevents::details

#endif