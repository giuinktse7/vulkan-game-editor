cmake_minimum_required(VERSION 3.5)

add_executable(
  vme_tests main.cpp position_test.cpp map_view_test.cpp item_test.cpp
            vulkan_window_test.cpp observable_item_test.cpp)

find_package(Catch2 CONFIG REQUIRED)
target_link_libraries(vme_tests PRIVATE Catch2::Catch2)

target_link_libraries(vme_tests PRIVATE common gui)

add_test(tests vme_tests)
