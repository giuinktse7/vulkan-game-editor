cmake_minimum_required(VERSION 3.16.0)

project(common VERSION 0.0.1 LANGUAGES CXX)

include(GNUInstallDirs)

include(CMakePackageConfigHelpers)

find_package(Vulkan REQUIRED)
find_package(protobuf CONFIG REQUIRED)
find_package(LibLZMA REQUIRED)
find_package(Stb REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(pugixml CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
# find_package(date CONFIG REQUIRED)
# target_link_libraries(common PRIVATE date::date date::date-tz)
find_path(NANO_SIGNAL_SLOT_INCLUDE_DIRS "nano_signal_slot.hpp")


set(HEADER_FILES
    common/config.h
    common/settings.h
    common/signal.h
    common/history/history.h
    common/history/history_action.h
    common/history/history_change.h
    common/history/thing_mutation.h
    common/camera.h
    common/const.h
    common/otbm.h
    common/debug.h
    common/definitions.h
    common/item_animation.h
    common/file.h
    common/error.h
    common/frame_group.h
    common/editor_action.h
    common/graphics/appearances.h
    common/graphics/appearance_types.h
    common/graphics/buffer.h
    common/graphics/compression.h
    common/graphics/device_manager.h
    common/graphics/engine.h
    common/graphics/protobuf/appearances.pb.h
    common/graphics/protobuf/map.pb.h
    common/graphics/protobuf/shared.pb.h
    common/graphics/resource-descriptor.h
    common/graphics/swapchain.h
    common/graphics/texture.h
    common/graphics/texture_atlas.h
    common/graphics/validation.h
    common/graphics/vertex.h
    common/graphics/vulkan_debug.h
    common/graphics/vulkan_helpers.h
    common/graphics/vulkan_screen_texture.h
    common/item.h
    common/item_data.h
    common/item_wrapper.h
    common/observable_item.h
    common/item_location.h
    common/items.h
    common/item_attribute.h
    common/item_type.h
    common/sprite_info.h
    common/logger.h
    common/vendor/lzma/7zTypes.h
    common/vendor/lzma/Alloc.h
    common/vendor/lzma/Compiler.h
    common/vendor/lzma/LzFind.h
    common/vendor/lzma/LzFindMt.h
    common/vendor/lzma/LzHash.h
    common/vendor/lzma/LzmaDec.h
    common/vendor/lzma/LzmaEnc.h
    common/vendor/lzma/LzmaLib.h
    common/vendor/lzma/Precomp.h
    common/vendor/lzma/Threads.h
    common/vendor/rollbear-visit/visit.hpp
    common/vendor/tsl/robin_growth_policy.h
    common/vendor/tsl/robin_hash.h
    common/vendor/tsl/robin_map.h
    common/vendor/tsl/robin_set.h
    common/vendor/result/result.h
    common/vendor/fts_fuzzy_match/fts_fuzzy_match.h
    # vendor/sol/sol.hpp vendor/sol/config.hpp vendor/sol/forward.hpp
    common/map.h
    common/save_map.h
    common/load_map.h
    common/map_renderer.h
    # map_renderer_qml.h
    common/map_copy_buffer.h
    common/map_view.h
    common/otb.h
    common/position.h
    common/quad_tree.h
    common/random.h
    common/selection.h
    common/tile.h
    common/tile_cover.h
    common/outfit.h
    common/tile_location.h
    common/minimap_colors.h
    common/time_util.h
    common/creature.h
    common/town.h
    common/item_palette.h
    common/tileset.h
    common/type_trait.h
    common/util.h
    common/lazy_object.h
    common/version.h
    common/octree.h
    common/brushes/brush.h
    common/brushes/raw_brush.h
    common/brushes/ground_brush.h
    common/brushes/border_brush.h
    common/brushes/border_brush_variation.h
    common/brushes/wall_brush.h
    common/brushes/doodad_brush.h
    common/brushes/creature_brush.h
    common/brushes/mountain_brush.h
    common/brushes/brush_loader.h
    common/lua/lua_state.h
    common/lua/luascript_interface.h
    common/lua/lua_brush.h
    common/concepts.h
)

set(SOURCE_FILES
    common//vendor/lzma/Alloc.c
    common//vendor/lzma/LzFind.c
    common//vendor/lzma/LzFindMt.c
    common//vendor/lzma/LzmaDec.c
    common//vendor/lzma/LzmaEnc.c
    common//vendor/lzma/LzmaLib.c
    common//vendor/lzma/Threads.c
    common/config.cpp
    common/settings.cpp
    common/history/history.cpp
    common/history/history_action.cpp
    common/history/history_change.cpp
    common/history/thing_mutation.cpp
    common/camera.cpp
    common/otbm.cpp
    common/item_animation.cpp
    common/file.cpp
    common/editor_action.cpp
    common/frame_group.cpp
    common/graphics/appearances.cpp
    common/graphics/buffer.cpp
    common/graphics/compression.cpp
    common/graphics/device_manager.cpp
    # graphics/engine.cpp
    common/graphics/protobuf/appearances.pb.cc
    common/graphics/protobuf/map.pb.cc
    common/graphics/protobuf/shared.pb.cc
    # graphics/resource-descriptor.cpp
    common/graphics/swapchain.cpp
    common/graphics/texture.cpp
    common/graphics/texture_atlas.cpp
    common/graphics/vulkan_debug.cpp
    common/graphics/vulkan_screen_texture.cpp
    common/item.cpp
    common/item_data.cpp
    common/item_wrapper.cpp
    common/observable_item.cpp
    common/item_location.cpp
    common/items.cpp
    common/item_attribute.cpp
    common/item_type.cpp
    common/sprite_info.cpp
    common/logger.cpp
    common/map.cpp
    common/save_map.cpp
    common/load_map.cpp
    common/map_renderer.cpp
    # map_renderer_qml.cpp
    common/map_copy_buffer.cpp
    common/map_view.cpp
    common/otb.cpp
    common/position.cpp
    common/quad_tree.cpp
    common/random.cpp
    common/selection.cpp
    common/tile.cpp
    common/tile_location.cpp
    common/tile_cover.cpp
    common/time_util.cpp
    common/creature.cpp
    common/town.cpp
    common/item_palette.cpp
    common/tileset.cpp
    common/util.cpp
    common/octree.cpp
    common/brushes/brush.cpp
    common/brushes/raw_brush.cpp
    common/brushes/ground_brush.cpp
    common/brushes/border_brush.cpp
    common/brushes/wall_brush.cpp
    common/brushes/border_brush_variation.cpp
    common/brushes/doodad_brush.cpp
    common/brushes/creature_brush.cpp
    common/brushes/mountain_brush.cpp
    common/brushes/brush_loader.cpp
    common/lua/lua_state.cpp
    common/lua/luascript_interface.cpp
    common/lua/lua_brush.cpp
)

add_library(${PROJECT_NAME} STATIC
    ${HEADER_FILES}
    ${SOURCE_FILES}
)
add_library(common::common ALIAS ${PROJECT_NAME})

# target_include_directories( ${PROJECT_NAME}
#     PUBLIC ${PROJECT_SOURCE_DIR}/include
# )
target_include_directories(${PROJECT_NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})



target_link_libraries(${PROJECT_NAME} PUBLIC Vulkan::Vulkan)
target_include_directories(${PROJECT_NAME} PUBLIC ${Vulkan_INCLUDE_DIRS})

target_link_libraries(${PROJECT_NAME} PRIVATE protobuf::libprotobuf)
# target_link_libraries(common PRIVATE ${LIBLZMA_LIBRARIES})
target_link_libraries(${PROJECT_NAME} PRIVATE LibLZMA::LibLZMA)

target_link_libraries(${PROJECT_NAME} PRIVATE pugixml)
target_link_libraries(${PROJECT_NAME} PRIVATE glm::glm)

target_include_directories(${PROJECT_NAME} PUBLIC ${NANO_SIGNAL_SLOT_INCLUDE_DIRS})

# By prefixing the installation paths with our name and version
# we can have multiple versions installed at the same time.
set(PROJECT_PREFIX ${PROJECT_NAME}-${common_VERSION})

# Defines the headers for this library.
# https://cmake.org/cmake/help/v3.11/command/target_include_directories.html
# target_include_directories(${PROJECT_NAME}
#         PUBLIC
#             # Used when building the library:
#             $<BUILD_INTERFACE:${common_SOURCE_DIR}/include>
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
# set(INSTALL_INCLUDE_DIR ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_PREFIX}/common)

# Define the configuration file locations.
# set(PROJECT_CONFIG_VERSION_FILE "${PROJECT_BINARY_DIR}/common-config-version.cmake")
# set(PROJECT_CONFIG_FILE         "${PROJECT_BINARY_DIR}/common-config.cmake")

# configure_package_config_file(cmake/common-config.cmake.in
#         ${PROJECT_CONFIG_FILE}
#         INSTALL_DESTINATION ${INSTALL_CONFIG_DIR})

# # This auto-generates a version file.
# write_basic_package_version_file(
#         ${PROJECT_CONFIG_VERSION_FILE}
#         COMPATIBILITY SameMajorVersion)

# The INSTALL(TARGET) command specifies where to install specific targets.
# By specifying EXPORT here, we add these targets to an export group.
# install(TARGETS common
#         EXPORT common-targets
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
# install(EXPORT common-targets
#         FILE common-targets.cmake
#         NAMESPACE common::
#         DESTINATION ${INSTALL_CONFIG_DIR})

# export(EXPORT common-targets
#        FILE ${CMAKE_CURRENT_BINARY_DIR}/common-targets.cmake
#        NAMESPACE common::)

