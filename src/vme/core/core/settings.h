#pragma once

enum class BorderBrushVariationType
{
    Detailed,
    General
};

struct Settings
{
    static BorderBrushVariationType BORDER_BRUSH_VARIATION;
    static bool AUTO_BORDER;
    static int UI_CHANGE_TIME_DELAY_MILLIS;
    static int DEFAULT_CREATURE_SPAWN_INTERVAL;

    static bool HIGHLIGHT_BRUSH_IN_PALETTE_ON_SELECT;
    /**
     * @brief Determines the offset where the brush item is to be inserted in a stack of items on a tile.
     * For example, if BRUSH_INSERTION_OFFSET is 1, the brush will place its item below the top item.
     *
     * This is useful to get finer control over the order of items on a tile, such as when performing manual bordering.
     */
    static int BRUSH_INSERTION_OFFSET;

    static bool RENDER_ANIMATIONS;

    static bool PLACE_MOUNTAIN_FEATURES;
};