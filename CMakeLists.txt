# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.25...4.0)

project(osevents
	VERSION 1.0.0
	LANGUAGES CXX
)

option(OSEVENTS_BUILD_SHARED "Whether to build osevents as a shared library" ${BUILD_SHARED_LIBS})
option(OSEVENTS_TESTS "Whether to build tests" ${PROJECT_IS_TOP_LEVEL})

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

if (NOT DEFINED CMAKE_CXX_STANDARD OR CMAKE_CXX_STANDARD VERSION_LESS 20)
	set(CMAKE_CXX_STANDARD 20)
endif()
set(CMAKE_CXX_EXTENSIONS OFF)

include(dependencies)

add_subdirectory(src)

if (OSEVENTS_TESTS)
	add_subdirectory(tests)
endif()

# TODO: set up export/install related boilerplate
