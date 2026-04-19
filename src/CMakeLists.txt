# SPDX-License-Identifier: BSD-3-Clause

include(GenerateExportHeader)

set(LIBTYPE "STATIC")
if (OSEVENTS_BUILD_SHARED)
	set(LIBTYPE "SHARED")
endif()

set(OSEVENTS_SCREENLOCK_EXE_DEFAULT "" CACHE STRING "Absolute path to the screen lock executable to keep polling for")

add_library(osevents ${LIBTYPE}
	"session_lock.cpp"

	"details/poll.cpp"
)

add_library(osevents::osevents ALIAS osevents)

if (NOT (OSEVENTS_SCREENLOCK_EXE_DEFAULT STREQUAL ""))
	target_compile_definitions(osevents PRIVATE OSEVENTS_SCREENLOCK_EXE_DEFAULT="${OSEVENTS_SCREENLOCK_EXE_DEFAULT}")
endif()

file(GLOB_RECURSE OSEVENTS_HEADER_FILES
	LIST_DIRECTORIES false
	CONFIGURE_DEPENDS
	"${PROJECT_SOURCE_DIR}/include/osevents/*.hpp"
)

generate_export_header(
	osevents
	EXPORT_FILE_NAME "${CMAKE_CURRENT_BINARY_DIR}/osevents/export_macros.hpp"
	INCLUDE_GUARD_NAME OSEVENTS_EXPORT_MACROS_HPP_
)

list(APPEND OSEVENTS_HEADER_FILES "${CMAKE_CURRENT_BINARY_DIR}/osevents/export_macros.hpp")

target_sources(osevents
	PUBLIC
		FILE_SET osevents_headers TYPE HEADERS
		BASE_DIRS "${PROJECT_SOURCE_DIR}/include" "${CMAKE_CURRENT_BINARY_DIR}"
		FILES "${OSEVENTS_HEADER_FILES}"
)

if (NOT PROJECT_IS_TOP_LEVEL)
	set(SYSTEM_FLAG "SYSTEM")
endif()

target_include_directories(osevents ${SYSTEM_FLAG}
	PUBLIC
		"$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
		"$<INSTALL_INTERFACE:${OSEVENTS_INSTALL_INCLUDEDIR}>"
)

set_target_properties(osevents
	PROPERTIES
		SOVERSION ${PROJECT_VERSION_MAJOR}
		VERSION ${PROJECT_VERSION}
)

if (WIN32)
	target_compile_definitions(osevents PRIVATE OSEVENTS_OS_WINDOWS)

	target_sources(osevents PRIVATE "details/windows.cpp")
	target_link_libraries(osevents PRIVATE WtsApi32)
elseif (APPLE)
	target_compile_definitions(osevents PRIVATE OSEVENTS_OS_APPLE)
elseif(LINUX)
	target_compile_definitions(osevents PRIVATE OSEVENTS_OS_LINUX)
elseif(BSD)
	target_compile_definitions(osevents PRIVATE OSEVENTS_OS_BSD)
else()
	message(FATAL_ERROR "Unknown target OS")
endif()
if (UNIX)
	target_compile_definitions(osevents PRIVATE OSEVENTS_OS_UNIX)
	target_sources(osevents PRIVATE "details/processes.cpp")
endif()

if (UNIX AND NOT APPLE)
	FetchContent_MakeAvailable(sdbus-c++)

	# This is necessary until we can depend on
	# https://github.com/Kistler-Group/sdbus-cpp/pull/522 being included in the obtained version
	if (TARGET SDBusCpp::sdbus-c++)
		target_link_libraries(osevents PRIVATE SDBusCpp::sdbus-c++)
	else()
		target_link_libraries(osevents PRIVATE sdbus-c++)
	endif()

	target_compile_definitions(osevents PRIVATE OSEVENTS_USE_DBUS)
	target_sources(osevents PRIVATE "details/dbus.cpp")
endif()
