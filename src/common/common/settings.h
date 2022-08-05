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

    static bool RENDER_ANIMATIONS;

    static bool PLACE_MOUNTAIN_FEATURES;
};