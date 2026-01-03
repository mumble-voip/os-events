# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.25...4.0)

project(osevents
	VERSION 1.0.0
	LANGUAGES CXX
)

option(OSEVENTS_BUILD_SHARED "Whether to build osevents as a shared library" ${BUILD_SHARED_LIBS})
option(OSEVENTS_TESTS "Whether to build tests" ${PROJECT_IS_TOP_LEVEL})
option(OSEVENTS_INSTALL "Whether to add os-events to the install target" ON)
option(OSEVENTS_EXPORT "Whether to export os-events targets to the build tree" ON)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

if (NOT DEFINED CMAKE_CXX_STANDARD OR CMAKE_CXX_STANDARD VERSION_LESS 20)
	set(CMAKE_CXX_STANDARD 20)
endif()
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

include(dependencies)
include(install_dirs)

add_subdirectory(src)

if (OSEVENTS_TESTS)
	add_subdirectory(tests)
endif()

if (OSEVENTS_INSTALL)
	include(CMakePackageConfigHelpers)

	configure_package_config_file(
		"${PROJECT_SOURCE_DIR}/cmake/osevents-config.cmake.in" "${CMAKE_CURRENT_BINARY_DIR}/osevents-config.cmake"
		INSTALL_DESTINATION "${OSEVENTS_INSTALL_CMAKEDIR}"
	)

	write_basic_package_version_file(
		"${CMAKE_CURRENT_BINARY_DIR}/osevents-config-version.cmake"
		VERSION ${PROJECT_VERSION}
		COMPATIBILITY SameMajorVersion
	)

	install(FILES
		"${CMAKE_CURRENT_BINARY_DIR}/osevents-config.cmake"
		"${CMAKE_CURRENT_BINARY_DIR}/osevents-config-version.cmake"
		DESTINATION "${OSEVENTS_INSTALL_CMAKEDIR}"
	)

	install(
		TARGETS osevents
		EXPORT osevents-targets
		RUNTIME DESTINATION "${OSEVENTS_INSTALL_BINDIR}"
			COMPONENT osevents_runtime
		LIBRARY DESTINATION "${OSEVENTS_INSTALL_LIBDIR}"
			COMPONENT osevents_runtime
			NAMELINK_COMPONENT osevents_development
		ARCHIVE DESTINATION "${OSEVENTS_INSTALL_LIBDIR}"
			COMPONENT osevents_development
		FILE_SET osevents_headers
	)

	install(
		EXPORT osevents-targets
		DESTINATION "${OSEVENTS_INSTALL_CMAKEDIR}"
		FILE osevents-targets.cmake
		NAMESPACE osevents::
		COMPONENT osevents_development
	)
endif()

if (OSEVENTS_EXPORT)
	export(
		TARGETS osevents
		NAMESPACE osevents::
		FILE "osevents-targets.cmake"
	)
endif()
