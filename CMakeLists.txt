cmake_minimum_required(VERSION 3.16.0)

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

set(CMAKE_PREFIX_PATH
    "D:/Programs/cpp/libraries/Qt6.1/6.1.0/msvc2019_64/lib/cmake")
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
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
# End QT

# include(FindLua)

find_package(Vulkan REQUIRED)
find_package(protobuf CONFIG REQUIRED)
find_package(liblzma CONFIG REQUIRED)
find_package(Stb REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(pugixml CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
# find_package(Lua REQUIRED) find_package(LuaJIT REQUIRED) find_package(sol2
# CONFIG REQUIRED)
find_path(NANO_SIGNAL_SLOT_INCLUDE_DIRS "nano_signal_slot.hpp")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(USE_LUAJIT true)

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

find_package(
  Qt6
  COMPONENTS Core Widgets Qml Quick SvgWidgets QuickControls2
  REQUIRED)

set(CommonSources
    src/config.h
    src/settings.h
    src/signal.h
    src/history/history.h
    src/history/history_action.h
    src/history/history_change.h
    src/history/thing_mutation.h
    src/camera.h
    src/const.h
    src/otbm.h
    src/debug.h
    src/definitions.h
    src/item_animation.h
    src/file.h
    src/frame_group.h
    src/editor_action.h
    src/graphics/appearances.h
    src/graphics/appearance_types.h
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
    src/item.h
    src/item_data.h
    src/item_wrapper.h
    src/observable_item.h
    src/item_location.h
    src/items.h
    src/item_attribute.h
    src/item_type.h
    src/sprite_info.h
    src/logger.h
    vendor/lzma/7zTypes.h
    vendor/lzma/Alloc.h
    vendor/lzma/Compiler.h
    vendor/lzma/LzFind.h
    vendor/lzma/LzFindMt.h
    vendor/lzma/LzHash.h
    vendor/lzma/LzmaDec.h
    vendor/lzma/LzmaEnc.h
    vendor/lzma/LzmaLib.h
    vendor/lzma/Precomp.h
    vendor/lzma/Threads.h
    vendor/rollbear-visit/visit.hpp
    vendor/tsl/robin_growth_policy.h
    vendor/tsl/robin_hash.h
    vendor/tsl/robin_map.h
    vendor/tsl/robin_set.h
    vendor/result/result.h
    vendor/fts_fuzzy_match/fts_fuzzy_match.h
    # vendor/sol/sol.hpp vendor/sol/config.hpp vendor/sol/forward.hpp
    src/map.h
    src/save_map.h
    src/load_map.h
    src/map_renderer.h
    src/map_copy_buffer.h
    src/map_view.h
    src/otb.h
    src/position.h
    src/quad_tree.h
    src/random.h
    src/selection.h
    src/tile.h
    src/tile_cover.h
    src/outfit.h
    src/tile_location.h
    src/minimap_colors.h
    src/time_util.h
    src/creature.h
    src/town.h
    src/item_palette.h
    src/tileset.h
    src/type_trait.h
    src/util.h
    src/lazy_object.h
    src/version.h
    src/octree.h
    src/brushes/brush.h
    src/brushes/raw_brush.h
    src/brushes/ground_brush.h
    src/brushes/border_brush.h
    src/brushes/border_brush_variation.h
    src/brushes/wall_brush.h
    src/brushes/doodad_brush.h
    src/brushes/creature_brush.h
    src/brushes/mountain_brush.h
    src/brushes/brush_loader.h
    src/lua/lua_state.h
    src/lua/luascript_interface.h
    src/lua/lua_brush.h
    src/concepts.h
    vendor/lzma/Alloc.c
    vendor/lzma/LzFind.c
    vendor/lzma/LzFindMt.c
    vendor/lzma/LzmaDec.c
    vendor/lzma/LzmaEnc.c
    vendor/lzma/LzmaLib.c
    vendor/lzma/Threads.c
    src/config.cpp
    src/settings.cpp
    src/history/history.cpp
    src/history/history_action.cpp
    src/history/history_change.cpp
    src/history/thing_mutation.cpp
    src/camera.cpp
    src/otbm.cpp
    src/item_animation.cpp
    src/file.cpp
    src/editor_action.cpp
    src/frame_group.cpp
    src/graphics/appearances.cpp
    src/graphics/buffer.cpp
    src/graphics/compression.cpp
    # src/graphics/device_manager.cpp src/graphics/engine.cpp
    src/graphics/protobuf/appearances.pb.cc
    src/graphics/protobuf/map.pb.cc
    src/graphics/protobuf/shared.pb.cc
    # src/graphics/resource-descriptor.cpp src/graphics/swapchain.cpp
    src/graphics/texture.cpp
    src/graphics/texture_atlas.cpp
    src/graphics/vulkan_debug.cpp
    src/item.cpp
    src/item_data.cpp
    src/item_wrapper.cpp
    src/observable_item.cpp
    src/item_location.cpp
    src/items.cpp
    src/item_attribute.cpp
    src/item_type.cpp
    src/sprite_info.cpp
    src/logger.cpp
    src/map.cpp
    src/save_map.cpp
    src/load_map.cpp
    src/map_renderer.cpp
    src/map_copy_buffer.cpp
    src/map_view.cpp
    src/otb.cpp
    src/position.cpp
    src/quad_tree.cpp
    src/random.cpp
    src/selection.cpp
    src/tile.cpp
    src/tile_location.cpp
    src/tile_cover.cpp
    src/time_util.cpp
    src/creature.cpp
    src/town.cpp
    src/item_palette.cpp
    src/tileset.cpp
    src/util.cpp
    src/octree.cpp
    src/brushes/brush.cpp
    src/brushes/raw_brush.cpp
    src/brushes/ground_brush.cpp
    src/brushes/border_brush.cpp
    src/brushes/wall_brush.cpp
    src/brushes/border_brush_variation.cpp
    src/brushes/doodad_brush.cpp
    src/brushes/creature_brush.cpp
    src/brushes/mountain_brush.cpp
    src/brushes/brush_loader.cpp
    src/lua/lua_state.cpp
    src/lua/luascript_interface.cpp
    src/lua/lua_brush.cpp)

add_library(common STATIC ${CommonSources})

# target_include_directories(common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_features(common PUBLIC cxx_std_20)
target_compile_features(common PRIVATE cxx_std_20)

target_link_libraries(common PUBLIC Vulkan::Vulkan)
target_include_directories(common PUBLIC ${Vulkan_INCLUDE_DIRS})

target_link_libraries(common PRIVATE protobuf::libprotobuf)
target_link_libraries(common PRIVATE ${LIBLZMA_LIBRARIES})

target_link_libraries(common PRIVATE pugixml)
target_link_libraries(common PRIVATE glm)

target_include_directories(common PUBLIC ${NANO_SIGNAL_SLOT_INCLUDE_DIRS})

# target_include_directories(common PRIVATE ${LUA_INCLUDE_DIR})
# target_link_libraries(common PRIVATE ${LUA_LIBRARIES})

# target_link_libraries(common PRIVATE sol2::sol2)

# target_compile_definitions(common PRIVATE SOL_USING_CXX_LUA=1)

# set_target_properties(common PROPERTIES LINK_FLAGS "/PROFILE")

set(GuiSources
    src/qt/enum_conversion.h
    src/qt/logging.h
    src/gui/gui.h
    # src/gui/borderless_window.h
    src/gui/main_application.h
    src/gui/border_layout.h
    src/gui/split_widget.h
    src/gui/mainwindow.h
    src/gui/map_view_widget.h
    src/gui/menu.h
    src/gui/qt_util.h
    src/gui/gui_thing_image.h
    src/gui/vulkan_window.h
    src/gui/map_tab_widget.h
    src/qt/qt_vulkan_info.h
    src/gui/draggable_item.h
    src/gui/properties/item_property_window.h
    src/gui/properties/property_container_tree.h
    src/gui/properties/container_model.h
    src/gui/properties/container_list_model.h
    src/gui/itempalette/item_palette_window.h
    src/gui/itempalette/item_palette_model.h
    src/gui/search_popup.h
    src/gui/minimap.h
    src/main.h
    src/main.cpp
    src/gui/main_application.cpp
    src/qt/enum_conversion.cpp
    src/qt/logging.cpp
    # src/gui/borderless_window.cpp
    src/gui/border_layout.cpp
    src/gui/split_widget.cpp
    src/gui/mainwindow.cpp
    src/gui/map_view_widget.cpp
    src/gui/menu.cpp
    src/gui/qt_util.cpp
    src/gui/gui_thing_image.cpp
    src/gui/vulkan_window.cpp
    src/gui/map_tab_widget.cpp
    src/gui/draggable_item.cpp
    src/gui/properties/item_property_window.cpp
    src/gui/properties/property_container_tree.cpp
    src/gui/properties/container_model.cpp
    src/gui/properties/container_list_model.cpp
    src/gui/itempalette/item_palette_window.cpp
    src/gui/itempalette/item_palette_model.cpp
    src/gui/search_popup.cpp
    src/gui/minimap.cpp
    src/qt/qt_vulkan_info.cpp)

# NOTE: All files that use the QT framework must be in the `gui` library,
# because it is the only target that links QT.
add_library(gui STATIC ${GuiSources})

target_link_libraries(gui PRIVATE common)
target_link_libraries(gui PRIVATE glm)

target_compile_features(gui PUBLIC cxx_std_20)
target_compile_features(gui PRIVATE cxx_std_20)

# QT
target_link_libraries(gui PUBLIC Qt6::Core Qt6::Widgets Qt6::Qml Qt6::Quick
                                 Qt6::SvgWidgets Qt6::QuickControls2)

# set_target_properties(gui PROPERTIES CMAKE_INCLUDE_CURRENT_DIR ON
# CMAKE_AUTOMOC ON) End QT

qt_add_resources(QT_RESOURCES resources/qml/qml.qrc)
qt_add_resources(QT_RESOURCES resources/style/style.qrc)
qt_add_resources(QT_RESOURCES resources/images/images.qrc)
qt_add_resources(QT_RESOURCES resources/shaders/shaders.qrc)
set(app_icon_resource_windows
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/vulkan-map-editor.rc")
qt_add_executable(main ${QT_RESOURCES} ${app_icon_resource_windows})

target_link_libraries(main PRIVATE common)
target_link_libraries(main PRIVATE gui)

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
