// SPDX-License-Identifier: BSD-3-Clause

#include <osevents/details/processes.hpp>

#include <cctype>
#include <charconv>
#include <filesystem>
#include <set>
#include <string>

#ifdef OSEVENTS_OS_UNIX
#	include <sys/types.h>
#endif

namespace osevents::details {

bool only_digits(std::string_view str) {
	for (char c : str) {
		if (!std::isdigit(c)) {
			return false;
		}
	}

	return true;
}

#ifdef OSEVENTS_OS_UNIX

std::set< Process > running_processes() {
	std::set< Process > processes;

	std::filesystem::path proc_dir = "/proc/";

	for (const std::filesystem::directory_entry &current : std::filesystem::directory_iterator(proc_dir)) {
		if (!current.is_directory()) {
			continue;
		}

		std::string pid_str = current.path().filename().string();
		if (!only_digits(pid_str)) {
			continue;
		}

		pid_t pid                  = 0;
		std::from_chars_result res = std::from_chars(pid_str.data(), pid_str.data() + pid_str.size(), pid);
		if (res.ec != std::errc{} || res.ptr != pid_str.data() + pid_str.size()) {
			continue;
		}

		std::error_code ec;
		std::filesystem::path exe_path = std::filesystem::read_symlink(current.path() / "exe", ec);

		if (ec) {
			continue;
		}

		processes.insert(Process{std::move(exe_path), pid});
	}

	return processes;
}

bool process_is_running(const Process &proc) {
	std::filesystem::path proc_path = std::filesystem::path("/proc/") / std::to_string(proc.pid);

	if (!std::filesystem::is_directory(proc_path)) {
		return false;
	}

	if (proc.exe_path.empty()) {
		return true;
	}

	std::error_code ec;
	std::filesystem::path exe_path = std::filesystem::read_symlink(proc_path / "exe", ec);

	if (ec) {
		return false;
	}

	return exe_path == proc.exe_path;
}

#endif

} // namespace osevents::details
