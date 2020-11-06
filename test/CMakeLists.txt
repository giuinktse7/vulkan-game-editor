cmake_minimum_required(VERSION 3.5)

add_executable(common_tests main.cpp position_test.cpp map_view_test.cpp
                            item_test.cpp)

find_package(Catch2 CONFIG REQUIRED)
target_link_libraries(common_tests PUBLIC Catch2::Catch2)

target_link_libraries(common_tests PRIVATE common)

add_test(tests common_tests)
