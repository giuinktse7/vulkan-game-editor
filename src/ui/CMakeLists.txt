cmake_minimum_required(VERSION 3.16.0)

project(qml_ui VERSION 0.0.1 LANGUAGES CXX)


include(GNUInstallDirs)

include(CMakePackageConfigHelpers)

set(CMAKE_AUTOMOC ON)


find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2)
find_package(glm CONFIG REQUIRED)
# find_package(common CONFIG REQUIRED)

set(HEADER_FILES
    src/qt_vulkan_info.h
)

set(SOURCE_FILES
    src/qt_vulkan_info.cpp
)

qt_add_library(qml_ui STATIC ${HEADER_FILES} ${SOURCE_FILES})
add_library(qml_ui::qml_ui ALIAS qml_ui)


target_link_libraries(${PROJECT_NAME} PUBLIC Qt::Core Qt::Gui Qt::Qml Qt::Quick Qt::QuickControls2)
target_link_libraries(${PROJECT_NAME} PUBLIC glm::glm)
target_link_libraries(${PROJECT_NAME} PUBLIC common::common)

target_include_directories(${PROJECT_NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

# set(PROJECT_PREFIX qml_ui-${qml_ui_VERSION})

# target_include_directories(qml_ui
#         PUBLIC
#             # Used when building the library:
#             $<BUILD_INTERFACE:${qml_ui_SOURCE_DIR}/include>
#             # Used when installing the library:
#             $<INSTALL_INTERFACE:include/${PROJECT_PREFIX}>
#         PRIVATE
#             # Used only when building the library:
#             src)

# Here we configure variables specifying the target directories.
# set(INSTALL_RUNTIME_DIR ${CMAKE_INSTALL_BINDIR})
# set(INSTALL_CONFIG_DIR  ${CMAKE_INSTALL_LIBDIR}/${PROJECT_PREFIX}/cmake)
# set(INSTALL_LIBRARY_DIR ${CMAKE_INSTALL_LIBDIR}/${PROJECT_PREFIX}/${CMAKE_BUILD_TYPE})
# set(INSTALL_ARCHIVE_DIR ${CMAKE_INSTALL_LIBDIR}/${PROJECT_PREFIX}/${CMAKE_BUILD_TYPE}/static)
# set(INSTALL_INCLUDE_DIR ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_PREFIX}/qml_ui)

# Define the configuration file locations.
# set(PROJECT_CONFIG_VERSION_FILE "${PROJECT_BINARY_DIR}/qml_ui-config-version.cmake")
# set(PROJECT_CONFIG_FILE         "${PROJECT_BINARY_DIR}/qml_ui-config.cmake")



# configure_package_config_file(cmake/qml_ui-config.cmake.in
#         ${PROJECT_CONFIG_FILE}
#         INSTALL_DESTINATION ${INSTALL_CONFIG_DIR})

# install(TARGETS qml_ui
#     EXPORT qml_ui-targets
#     RUNTIME DESTINATION ${INSTALL_RUNTIME_DIR}
#     LIBRARY DESTINATION ${INSTALL_LIBRARY_DIR}
#     ARCHIVE DESTINATION ${INSTALL_ARCHIVE_DIR})

# This auto-generates a version file.
# write_basic_package_version_file(
#         ${PROJECT_CONFIG_VERSION_FILE}
#         COMPATIBILITY SameMajorVersion)

# The INSTALL(TARGET) command specifies where to install specific targets.
# By specifying EXPORT here, we add these targets to an export group.
# install(TARGETS qml_ui
#         EXPORT qml_ui-targets
#         RUNTIME DESTINATION ${INSTALL_RUNTIME_DIR}
#         LIBRARY DESTINATION ${INSTALL_LIBRARY_DIR}
#         ARCHIVE DESTINATION ${INSTALL_ARCHIVE_DIR})

# The INSTALL(FILES) command specifies where to install specific files, such as headers.
# install(FILES ${HEADER_FILES}
#         DESTINATION ${INSTALL_INCLUDE_DIR})

# The INSTALL(FILES) command specifies where to install specific files, such as headers.
# install(FILES
#             ${PROJECT_CONFIG_VERSION_FILE}
#             ${PROJECT_CONFIG_FILE}
#         DESTINATION ${INSTALL_CONFIG_DIR})

# The INSTALL(EXPORT) command installs a previously defined export group to a script.
# install(EXPORT qml_ui-targets
#         FILE qml_ui-targets.cmake
#         NAMESPACE qml_ui::
#         DESTINATION ${INSTALL_CONFIG_DIR})


