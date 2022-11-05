find_package(Catch2 3 REQUIRED)

set(SOURCE_FILES
    item_test.cpp
    map_view_test.cpp
    observable_item_test.cpp
    position_test.cpp
)


add_executable(core-tests ${SOURCE_FILES})
target_link_libraries(core-tests PRIVATE core Catch2::Catch2WithMain)