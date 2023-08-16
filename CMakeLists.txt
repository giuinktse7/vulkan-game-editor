cmake_minimum_required(VERSION 3.16)

# Required to enable testing for link-time optimization.
# https://cmake.org/cmake/help/v3.11/module/CheckIPOSupported.html
include(CheckIPOSupported)

# Use link-time optimization if allowed.
check_ipo_supported(RESULT ipo_supported)
if(ipo_supported)
    set_property(GLOBAL PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

# set(CMAKE_PREFIX_PATH "C:/Qt/6.3.1/msvc2019_64/lib/cmake")
 


# If no build type was specified, set it to Release.
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING
            "Choose the type of build, options are: None Debug Release."
            FORCE)
endif(NOT CMAKE_BUILD_TYPE)

# vcpkg setup
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE
      "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
      CACHE STRING "")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable Vulkan debugging layer
set(QSG_RHI_DEBUG_LAYER 1)

project(vme VERSION 1.0.0 LANGUAGES C CXX)


find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2)
# qt_standard_project_setup()

add_subdirectory(src/vme/core)
add_subdirectory(src/vme/AppComponents)
add_subdirectory(src/vme/app)
