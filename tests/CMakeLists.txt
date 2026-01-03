# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.21...4.0)

project(osevents-tests
	LANGUAGES CXX
)

if (PROJECT_IS_TOP_LEVEL)
	find_package(osevents REQUIRED)
endif()

add_executable(osevents_test "test.cpp")
target_link_libraries(osevents_test PRIVATE osevents::osevents)
