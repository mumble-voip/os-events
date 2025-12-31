# SPDX-License-Identifier: BSD-3-Clause

include(GNUInstallDirs)

set(LIBTYPE "STATIC")
if (OSEVENTS_BUILD_SHARED)
	set(LIBTYPE "SHARED")
endif()

add_library(osevents ${LIBTYPE}
	"session_lock.cpp"

	"details/dbus.cpp"
)

target_include_directories(osevents
	PUBLIC
		"$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
		"$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCCLUDEDIR}>"
)

if (WIN32)
	target_compile_definitions(osevents PRIVATE OSEVENTS_OS_WINDOWS)
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
endif()
