cmake_minimum_required(VERSION 3.16.0)

# This will allow us to print a feature summary.
# https://cmake.org/cmake/help/v3.11/module/FeatureSummary.html
include(FeatureSummary)

# Required to enable testing for link-time optimization.
# https://cmake.org/cmake/help/v3.11/module/CheckIPOSupported.html
include(CheckIPOSupported)

# Use link-time optimization if allowed.
check_ipo_supported(RESULT ipo_supported)
if(ipo_supported)
    set_property(GLOBAL PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

# We attempt to use ccache to speed up the build.
find_program(CCACHE_FOUND "ccache")
if(CCACHE_FOUND)
    message(STATUS "Using ccache for building.")
    set_property(GLOBAL PROPERTY LAUNCH_COMPILE ccache)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif(CCACHE_FOUND)


# If no build type was specified, set it to Release.
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING
            "Choose the type of build, options are: None Debug Release."
            FORCE)
endif(NOT CMAKE_BUILD_TYPE)

# If no installation prefix is given manually, install locally.
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE STRING
            "The install location"
            FORCE)
endif(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)

#[[
Build instructions:
1. Navigate to ./build.

2. To generate Cmake build files (choose appropriate generator, platform and compiler):
cmake ../ -G "Visual Studio 16 2019" -A x64 -T ClangCL

3. To build the project for Debug:
cmake --build .
4. To build the project for Release:
cmake --build . --config release
]]
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE
      "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
      CACHE STRING "")
endif()

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_PREFIX_PATH "C:/Qt/6.3.1/msvc2019_64/lib/cmake")
# set(CMAKE_CXX_CLANG_TIDY "clang-tidy;-format-style='file'")

# set(ARCH "-target x86_64-w64-mingw32") set(CMAKE_ASM_FLAGS "${ARCH}")
# set(CMAKE_C_FLAGS "${ARCH}") set(CMAKE_CXX_FLAGS "${ARCH}")

project(
  VulkanGameEditor
  VERSION 0.0.1 
  LANGUAGES C CXX)

enable_testing()
add_subdirectory("test")

# set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# QT Specific
# set(CMAKE_AUTORCC ON)
# set(CMAKE_AUTOUIC ON)
# End QT

# include(FindLua)

# find_package(Vulkan REQUIRED)
# find_package(protobuf CONFIG REQUIRED)
# find_package(liblzma CONFIG REQUIRED)
find_package(Stb REQUIRED)
# find_package(glm CONFIG REQUIRED)
# find_package(pugixml CONFIG REQUIRED)
# find_package(nlohmann_json CONFIG REQUIRED)
# find_package(Lua REQUIRED) find_package(LuaJIT REQUIRED) find_package(sol2
# CONFIG REQUIRED)
# find_path(NANO_SIGNAL_SLOT_INCLUDE_DIRS "nano_signal_slot.hpp")

# find_package(OpenSSL)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# set(USE_LUAJIT true)

# Selects LuaJIT if user defines or auto-detected
if(DEFINED USE_LUAJIT AND NOT USE_LUAJIT)
  set(FORCE_LUAJIT ${USE_LUAJIT})
else()
  find_package(LuaJIT)
  set(FORCE_LUAJIT ${LuaJIT_FOUND})
endif()
option(USE_LUAJIT "Use LuaJIT" ${FORCE_LUAJIT})

if(FORCE_LUAJIT)
  if(APPLE)
    set(CMAKE_EXE_LINKER_FLAGS "-pagezero_size 10000 -image_base 100000000")
  endif()
else()
  find_package(Lua REQUIRED)
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Qml Quick SvgWidgets QuickControls2)
qt_standard_project_setup()

# NOTE: All files that use the QT framework must be in the `ui` library,
# because it is the only target that links QT.

add_subdirectory(src/common)
add_subdirectory(src/ui)

# qt_add_resources(QT_RESOURCES resources/qml/qml.qrc)
# qt_add_resources(QT_RESOURCES resources/style/style.qrc)
# qt_add_resources(QT_RESOURCES resources/images/images.qrc)
# qt_add_resources(QT_RESOURCES resources/shaders/shaders.qrc)
set(app_icon_resource_windows
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/vulkan-map-editor.rc")

set(MainSources
    src/main.h
    src/main.cpp
    vulkansquircle.h
    vulkansquircle.cpp
    qml_map_item.h
    qml_map_item.cpp
)

qt_add_executable(main ${MainSources} ${app_icon_resource_windows})

qt_add_qml_module(main
    URI VulkanGameEditor
    VERSION 1.0
    QML_FILES
        ui/main.qml
    RESOURCE_PREFIX /vulkangameeditor
    NO_RESOURCE_TARGET_PATH
)



# target_link_libraries(main PUBLIC Qt::Core Qt::Gui Qt::Widgets Qt::Qml Qt::Quick Qt::SvgWidgets Qt::QuickControls2)


# if (DEFINED OpenSSL_LIBRARY)
#   target_link_libraries(main PRIVATE OpenSSL::SSL OpenSSL::Crypto)
# endif()

target_link_libraries(main PRIVATE common::common)
target_link_libraries(main PUBLIC qml_ui::qml_ui)

target_compile_features(main PUBLIC cxx_std_20)
target_compile_features(main PRIVATE cxx_std_20)


# Move required files post-build
add_custom_command(
  TARGET main
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/resources/
          $<TARGET_FILE_DIR:main>/resources)

add_custom_command(
  TARGET main
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/data/
          $<TARGET_FILE_DIR:main>/data)

add_custom_command(
  TARGET main
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/shaders/
          $<TARGET_FILE_DIR:main>/shaders)
