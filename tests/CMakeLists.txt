# SPDX-License-Identifier: BSD-3-Clause

add_executable(osevents_test "test.cpp")
target_link_libraries(osevents_test PRIVATE osevents::osevents)
