cmake_minimum_required(VERSION 3.16.0)

find_package(Vulkan REQUIRED)
find_package(Protobuf CONFIG REQUIRED)
find_package(LibLZMA REQUIRED)
find_package(Stb REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(pugixml CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
# find_package(date CONFIG REQUIRED)
# target_link_libraries(common PRIVATE date::date date::date-tz)
find_path(NANO_SIGNAL_SLOT_INCLUDE_DIRS "nano_signal_slot.hpp")
find_package(spdlog CONFIG REQUIRED)


set(HEADER_FILES
    core/log.h
    core/config.h
    core/settings.h
    core/signal.h
    core/history/history.h
    core/history/history_action.h
    core/history/history_change.h
    core/history/thing_mutation.h
    core/camera.h
    core/const.h
    core/otbm.h
    core/debug.h
    core/definitions.h
    core/item_animation.h
    core/file.h
    core/error.h
    core/frame_group.h
    core/editor_action.h
    core/graphics/appearances.h
    core/graphics/appearance_types.h
    core/graphics/buffer.h
    core/graphics/compression.h
    core/graphics/device_manager.h
    core/graphics/engine.h
    core/graphics/protobuf/appearances.pb.h
    core/graphics/protobuf/map.pb.h
    core/graphics/protobuf/shared.pb.h
    core/graphics/resource-descriptor.h
    # core/graphics/swapchain.h
    core/graphics/texture.h
    core/graphics/texture_atlas.h
    core/graphics/validation.h
    core/graphics/vertex.h
    core/graphics/vulkan_debug.h
    core/graphics/vulkan_helpers.h
    core/graphics/vulkan_screen_texture.h
    core/item.h
    core/item_data.h
    core/item_wrapper.h
    core/observable_item.h
    core/item_location.h
    core/items.h
    core/item_attribute.h
    core/item_type.h
    core/sprite_info.h
    core/logger.h
    core/vendor/lzma/7zTypes.h
    core/vendor/lzma/Alloc.h
    core/vendor/lzma/Compiler.h
    core/vendor/lzma/LzFind.h
    core/vendor/lzma/LzFindMt.h
    core/vendor/lzma/LzHash.h
    core/vendor/lzma/LzmaDec.h
    core/vendor/lzma/LzmaEnc.h
    core/vendor/lzma/LzmaLib.h
    core/vendor/lzma/Precomp.h
    core/vendor/lzma/Threads.h
    core/vendor/rollbear-visit/visit.hpp
    core/vendor/tsl/robin_growth_policy.h
    core/vendor/tsl/robin_hash.h
    core/vendor/tsl/robin_map.h
    core/vendor/tsl/robin_set.h
    core/vendor/result/result.h
    core/vendor/fts_fuzzy_match/fts_fuzzy_match.h
    # vendor/sol/sol.hpp vendor/sol/config.hpp vendor/sol/forward.hpp
    core/map.h
    core/save_map.h
    core/load_map.h
    core/map_renderer.h
    core/render/quad_mesh.h
    # map_renderer_qml.h
    core/map_copy_buffer.h
    core/map_view.h
    core/otb.h
    core/position.h
    core/quad_tree.h
    core/random.h
    core/selection.h
    core/tile.h
    core/tile_cover.h
    core/outfit.h
    core/tile_location.h
    core/minimap_colors.h
    core/time_util.h
    core/creature.h
    core/town.h
    core/item_palette.h
    core/tileset.h
    core/type_trait.h
    core/util.h
    core/lazy_object.h
    core/version.h
    core/octree.h
    core/brushes/brushes.h
    core/brushes/brushes.cpp
    core/brushes/brush.h
    core/brushes/raw_brush.h
    core/brushes/ground_brush.h
    core/brushes/border_brush.h
    core/brushes/border_brush_variation.h
    core/brushes/wall_brush.h
    core/brushes/doodad_brush.h
    core/brushes/creature_brush.h
    core/brushes/mountain_brush.h
    core/brushes/brush_loader.h
    core/lua/lua_state.h
    core/lua/luascript_interface.h
    core/lua/lua_brush.h
    core/concepts.h
)

set(SRC_FILES
    core/vendor/lzma/Alloc.c
    core/vendor/lzma/LzFind.c
    core/vendor/lzma/LzFindMt.c
    core/vendor/lzma/LzmaDec.c
    core/vendor/lzma/LzmaEnc.c
    core/vendor/lzma/LzmaLib.c
    core/vendor/lzma/Threads.c
    core/config.cpp
    core/settings.cpp
    core/history/history.cpp
    core/history/history_action.cpp
    core/history/history_change.cpp
    core/history/thing_mutation.cpp
    core/camera.cpp
    core/otbm.cpp
    core/item_animation.cpp
    core/file.cpp
    core/editor_action.cpp
    core/frame_group.cpp
    core/graphics/appearances.cpp
    core/graphics/buffer.cpp
    core/graphics/compression.cpp
    core/graphics/device_manager.cpp
    # graphics/engine.cpp
    core/graphics/protobuf/appearances.pb.cc
    core/graphics/protobuf/map.pb.cc
    core/graphics/protobuf/shared.pb.cc
    # graphics/resource-descriptor.cpp
    # core/graphics/swapchain.cpp
    core/graphics/texture.cpp
    core/graphics/texture_atlas.cpp
    core/graphics/vulkan_debug.cpp
    core/graphics/vulkan_screen_texture.cpp
    core/item.cpp
    core/item_data.cpp
    core/item_wrapper.cpp
    core/observable_item.cpp
    core/item_location.cpp
    core/items.cpp
    core/item_attribute.cpp
    core/item_type.cpp
    core/sprite_info.cpp
    core/logger.cpp
    core/map.cpp
    core/save_map.cpp
    core/load_map.cpp
    core/map_renderer.cpp
    core/render/quad_mesh.cpp
    # map_renderer_qml.cpp
    core/map_copy_buffer.cpp
    core/map_view.cpp
    core/otb.cpp
    core/position.cpp
    core/quad_tree.cpp
    core/random.cpp
    core/selection.cpp
    core/tile.cpp
    core/tile_location.cpp
    core/tile_cover.cpp
    core/time_util.cpp
    core/creature.cpp
    core/town.cpp
    core/item_palette.cpp
    core/tileset.cpp
    core/util.cpp
    core/octree.cpp
    core/brushes/brush.cpp
    core/brushes/raw_brush.cpp
    core/brushes/ground_brush.cpp
    core/brushes/border_brush.cpp
    core/brushes/wall_brush.cpp
    core/brushes/border_brush_variation.cpp
    core/brushes/doodad_brush.cpp
    core/brushes/creature_brush.cpp
    core/brushes/mountain_brush.cpp
    core/brushes/brush_loader.cpp
    core/lua/lua_state.cpp
    core/lua/luascript_interface.cpp
    core/lua/lua_brush.cpp
)

add_library(core STATIC
    ${HEADER_FILES}
    ${SRC_FILES}
)

target_include_directories(core INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(core PUBLIC Vulkan::Vulkan)
target_include_directories(core PUBLIC ${Vulkan_INCLUDE_DIRS})

target_link_libraries(core PRIVATE protobuf::libprotobuf)
# target_link_libraries(common PRIVATE ${LIBLZMA_LIBRARIES})
target_link_libraries(core PRIVATE LibLZMA::LibLZMA)

target_link_libraries(core PRIVATE pugixml)
target_link_libraries(core PRIVATE glm::glm)
target_link_libraries(core PUBLIC spdlog::spdlog)
target_link_libraries(core PRIVATE nlohmann_json::nlohmann_json)

target_include_directories(core PUBLIC ${NANO_SIGNAL_SLOT_INCLUDE_DIRS})

add_subdirectory(test)