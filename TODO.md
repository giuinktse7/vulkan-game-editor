# TODO

Current goals:

-   [ ] Light effects (For example from torches)

    -   Links:
        -   [StackOverflow - Combining and drawing 2D lights in OpenGL](https://gamedev.stackexchange.com/questions/135458/combining-and-drawing-2d-lights-in-opengl)
        -   [Vulkan tutorial - Shader modules](https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Shader_modules)
        -   For the light blending:
            -   https://stackoverflow.com/questions/67728826/blend-glistening-lights
    -   Steps:
        -   Add push constants when creating the lights Vk Pipeline
        -   Use push constants in the draw call for the light
            -   Include position, color, intensity (See [Light](https://github.com/OTAcademy/RME/blob/61df85c71bd51b218d28457bbcb5191d201480ae/source/light_drawer.cpp#L57), [Intensity](https://github.com/OTAcademy/RME/blob/61df85c71bd51b218d28457bbcb5191d201480ae/source/light_drawer.h#L47))
        -   [x] Use the UBO in the vertex shader
        -   [x] Use push constants in the vertex shader
        -   Pass relevant data to the fragment shader
        -   Implement the fragment shader

-   [ ] (!) Fix bug: Saving the first map view when the editor opens does not work. It saves an empty map.
-   [ ] Use Settings::BRUSH_INSERTION_OFFSET when rendering a Raw Brush preview
-   [ ] Add right-click context menu items:
    -   Select raw brush
    -   Select ground brush
    -   Select border brush
    -   Select doodad brush
    -   Select wall brush
-   [ ] Render map as minimap (setting to only render minimap ground colors as the actual map) (RME has this feature)

## Palettes

## Crashes

## Rendering

## Map Rendering

-   [x] Add possbility for a CreatureType to have the look of an item
-   [x] ~~Implement animation~~
-   [x] ~~Handle stackable appearances~~
-   [x] ~~Handle appearances with elevation~~
-   [x] ~~Feature?: When removing in area (Drag while Ctrl+Shift), show selection rectangle (red tinted?) with the selected item at mouse cursor ONLY.~~
-   [x] ~~Feature: Instant feedback for what items will be removed when removing in area (Drag while Ctrl+Shift).~~
-   [ ] Animations
    -   Implementation:
        -   When rendering map, store the lowest animation delay D for next animation for any rendered item.
        -   If any animation was found, queue another render with delay D. Otherwise, no render queueing is necessary.
    -   [x] Only update visible animations.
    -   [ ] Do not render animations when zoomed out further than a certain level.

## Editing functionality

-   [ ] topItem selected: RELEASE deselects, but ONLY if mouse has been outside of this tile since the PRESS event.
-   [ ] Reorder items on a tile.
-   [ ] (_Stretch_): reorder an item group among selected tiles. For example, move grass borders one level up.
-   [x] ~~Move selected items~~
-   [x] ~~topItem **not** selected: PRESS selects.~~
-   [x] ~~BUG: A selection can be moved out of bounds. It should clamp to the map size, [and maybe to max/min floor?]~~

## Optimization

### GUI

-   [x] Drag & Drop of item palettes
-   [ ] Snap to left/bottom/right
-   [ ] Snap to other itempalettes - Only if the itempalette is not already snapped to a side
-   [ ] Palette ComboBox
    -   [x] Render the palette names
    -   [x] Implement changing palette
    -   [x] Implement changing tileset

## General

-   [ ] System for customizable keybindings

## Scripting

-   [ ] Use sol:
    -   Tutorial: https://sol2.readthedocs.io/en/latest/tutorial/all-the-things.html
    -   GitHub: https://github.com/ThePhD/sol2

## Map

-   [ ] Investigate whether the map can be stored more efficiently. Place to start: [quadtree](https://en.wikipedia.org/wiki/Quadtree).

## CEF

Explore using Chromium Embedded Framework for UI

### Possible lead

-   Allegedly uses Vulkan with CEF: https://github.com/dsleep/SPP
-   Someone asking the same question: https://www.reddit.com/r/vulkan/comments/85xjp8/cef_ui_integration_in_vulkan/
-   Example to render texture in Vulkan: https://github.com/PacktPublishing/Vulkan-Cookbook/blob/master/Samples/Source%20Files/12%20Advanced%20Rendering%20Techniques/05-Rendering_a_fullscreen_quad_for_postprocessing/main.cpp

Old:
https://github.com/0x61726b/cef3d
https://github.com/qwertzui11/cef_osr
https://github.com/acristoffers/CEF3SimpleSample
