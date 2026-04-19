// SPDX-License-Identifier: BSD-3-Clause

#ifndef OSEVENTS_DETAILS_PROCESSES_HPP_
#define OSEVENTS_DETAILS_PROCESSES_HPP_

#include <compare>
#include <filesystem>
#include <set>

#ifdef OSEVENTS_OS_UNIX
#	include <sys/types.h>
#endif

namespace osevents::details {

#ifdef OSEVENTS_OS_UNIX

/**
 * Defines a process to consist of its PID along with the path to the associated executable
 */
struct Process {
	std::filesystem::path exe_path = {};
	pid_t pid                      = {};

	std::strong_ordering operator<=>(const Process &) const = default;
	bool operator==(const Process &) const                  = default;
};

/**
 * @returns Collection of all currently running processes
 */
std::set< Process > running_processes();

/**
 * returns Whether the given process is (still) running
 */
bool process_is_running(const Process &proc);

#endif

} // namespace osevents::details

#endif
