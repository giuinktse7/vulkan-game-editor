cmake_minimum_required(VERSION 3.5)

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

set(CMAKE_PREFIX_PATH ${QT_CMAKE_FOLDER})

# set(ARCH "-target x86_64-w64-mingw32") set(CMAKE_ASM_FLAGS "${ARCH}")
# set(CMAKE_C_FLAGS "${ARCH}") set(CMAKE_CXX_FLAGS "${ARCH}")

project(
  VulkanGameEditor
  VERSION 0.0.1
  LANGUAGES C CXX)

# QT Specific
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
# End QT

find_package(Vulkan REQUIRED)
find_package(protobuf CONFIG REQUIRED)
find_package(liblzma CONFIG REQUIRED)
find_package(stb REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(pugixml CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

find_package(
  Qt5
  COMPONENTS Core Widgets Svg
  REQUIRED)

add_executable(
  main
  src/action/action.h
  src/camera.h
  src/const.h
  src/debug.h
  src/definitions.h
  src/ecs/component_array.h
  src/ecs/ecs.h
  src/ecs/entity.h
  src/ecs/item_animation.h
  src/file.h
  src/graphics/appearances.h
  src/graphics/batch_item_draw.h
  src/graphics/buffer.h
  src/graphics/compression.h
  src/graphics/device_manager.h
  src/graphics/engine.h
  src/graphics/protobuf/appearances.pb.h
  src/graphics/protobuf/map.pb.h
  src/graphics/protobuf/shared.pb.h
  src/graphics/resource-descriptor.h
  src/graphics/swapchain.h
  src/graphics/texture.h
  src/graphics/texture_atlas.h
  src/graphics/validation.h
  src/graphics/vertex.h
  src/graphics/vulkan_debug.h
  src/graphics/vulkan_helpers.h
  src/gui/borderless_window.h
  src/gui/border_layout.h
  src/gui/item_list.h
  src/gui/mainwindow.h
  src/gui/map_view_widget.h
  src/gui/menu.h
  src/gui/qt_util.h
  src/gui/vulkan_window.h
  src/gui/map_tab_widget.h
  src/item.h
  src/items.h
  src/item_attribute.h
  src/item_type.h
  src/logger.h
  src/lzma/7zTypes.h
  src/lzma/Alloc.h
  src/lzma/Compiler.h
  src/lzma/LzFind.h
  src/lzma/LzFindMt.h
  src/lzma/LzHash.h
  src/lzma/LzmaDec.h
  src/lzma/LzmaEnc.h
  src/lzma/LzmaLib.h
  src/lzma/Precomp.h
  src/lzma/Threads.h
  src/main.h
  src/map.h
  src/map_io.h
  src/map_renderer.h
  src/map_view.h
  src/otb.h
  src/position.h
  src/qt/logging.h
  src/quad_tree.h
  src/random.h
  src/selection.h
  src/tile.h
  src/tile_location.h
  src/time_point.h
  src/town.h
  src/trianglerenderer.h
  src/type_trait.h
  src/ui_mainwindow.h
  src/util.h
  src/version.h
  src/lzma/Alloc.c
  src/lzma/LzFind.c
  src/lzma/LzFindMt.c
  src/lzma/LzmaDec.c
  src/lzma/LzmaEnc.c
  src/lzma/LzmaLib.c
  src/lzma/Threads.c
  src/action/action.cpp
  src/camera.cpp
  src/ecs/ecs.cpp
  src/ecs/item_animation.cpp
  src/file.cpp
  src/graphics/appearances.cpp
  src/graphics/batch_item_draw.cpp
  src/graphics/buffer.cpp
  src/graphics/compression.cpp
  src/graphics/device_manager.cpp
  src/graphics/engine.cpp
  src/graphics/protobuf/appearances.pb.cc
  src/graphics/protobuf/map.pb.cc
  src/graphics/protobuf/shared.pb.cc
  src/graphics/resource-descriptor.cpp
  src/graphics/swapchain.cpp
  src/graphics/texture.cpp
  src/graphics/texture_atlas.cpp
  src/graphics/vulkan_debug.cpp
  src/graphics/vulkan_helpers.cpp
  src/gui/borderless_window.cpp
  src/gui/border_layout.cpp
  src/gui/item_list.cpp
  src/gui/mainwindow.cpp
  src/gui/map_view_widget.cpp
  src/gui/menu.cpp
  src/gui/qt_util.cpp
  src/gui/vulkan_window.cpp
  src/gui/map_tab_widget.cpp
  src/item.cpp
  src/items.cpp
  src/item_attribute.cpp
  src/item_type.cpp
  src/logger.cpp
  src/main.cpp
  src/map.cpp
  src/map_io.cpp
  src/map_renderer.cpp
  src/map_view.cpp
  src/otb.cpp
  src/position.cpp
  src/qt/logging.cpp
  src/quad_tree.cpp
  src/random.cpp
  src/selection.cpp
  src/tile.cpp
  src/tile_location.cpp
  src/time_point.cpp
  src/town.cpp
  src/trianglerenderer.cpp
  src/util.cpp)

target_compile_features(main PRIVATE cxx_std_17)

target_link_libraries(main PRIVATE Vulkan::Vulkan)
target_include_directories(main PRIVATE ${Vulkan_INCLUDE_DIRS})

target_link_libraries(main PRIVATE protobuf::libprotobuf)
target_link_libraries(main PRIVATE ${LIBLZMA_LIBRARIES})

target_link_libraries(main PRIVATE pugixml)

# QT
target_link_libraries(main PUBLIC Qt::Core)
target_link_libraries(main PUBLIC Qt::Widgets)
target_link_libraries(main PUBLIC Qt::Svg)
# End QT

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
