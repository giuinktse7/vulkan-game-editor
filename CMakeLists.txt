cmake_minimum_required(VERSION 3.16)

project(vme VERSION 1.0.0 LANGUAGES C CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Required to enable testing for link-time optimization.
# https://cmake.org/cmake/help/v3.11/module/CheckIPOSupported.html
include(CheckIPOSupported)

# Use link-time optimization if allowed.
check_ipo_supported(RESULT ipo_supported)
if(ipo_supported)
    set_property(GLOBAL PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

set(CMAKE_PREFIX_PATH "C:/Qt/6.9.0/msvc2022_64/lib/cmake")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_definitions(${Qt6Core_DEFINITIONS})

# Shaders
include(CompileShaders)

set(COMPILE_SHADERS_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/src/vme/app")
message(STATUS "Shader output directory: ${COMPILE_SHADERS_OUTPUT_DIR}")

# reset the global COMPILED_SHADERS var
set(COMPILED_SHADERS)


set(SHADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/shader.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/shader.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/light/light.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/light/light.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/sprite-atlas.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/sprite-atlas.frag
)

set(GLSL_FLAGS --target-env vulkan1.3)

compile_shaders("${SHADERS}" OUTPUT_DIR "${COMPILE_SHADERS_OUTPUT_DIR}")
add_custom_target(compile_shaders ALL DEPENDS ${COMPILED_SHADERS})

# Copy data directory to build directory
add_custom_target(copy_data ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/data
        ${CMAKE_CURRENT_BINARY_DIR}/src/vme/app/data
    COMMENT "Copying data directory to build location"
)

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


# Enable Vulkan debugging layer
set(QSG_RHI_DEBUG_LAYER 1)



find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2)
# qt_standard_project_setup()

add_subdirectory(src/vme/core)
add_subdirectory(src/vme/AppComponents)
add_subdirectory(src/vme/app)
