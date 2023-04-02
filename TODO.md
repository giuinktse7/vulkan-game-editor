# TODO

Current goals:

-   [ ] (!) Fix bug: Saving the first map view when the editor opens does not work. It saves an empty map.
-   [ ] Use Settings::BRUSH_INSERTION_OFFSET when rendering a Raw Brush preview
-   [ ] Add right-click context menu items:
    -   Select raw brush
    -   Select ground brush
    -   Select border brush
    -   Select doodad brush
    -   Select wall brush

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
-   [ ] Light effects (For example from torches)

## Editing functionality

-   [ ] topItem selected: RELEASE deselects, but ONLY if mouse has been outside of this tile since the PRESS event.
-   [ ] Reorder items on a tile.
-   [ ] (_Stretch_): reorder an item group among selected tiles. For example, move grass borders one level up.
-   [x] ~~Move selected items~~
-   [x] ~~topItem **not** selected: PRESS selects.~~
-   [x] ~~BUG: A selection can be moved out of bounds. It should clamp to the map size, [and maybe to max/min floor?]~~

## Optimization

### GUI

-   [ ] Drag & Drop of item palettes
-   [ ] Snap to left/bottom/right
-   [ ] Snap to other itempalettes - Only if the itempalette is not already snapped to a side
-   [ ] Palette ComboBox
    -   [x] Render the palette names
    -   [ ] Implement changing palette
    -   [ ] Implement changing tileset

## General

-   [ ] System for customizable keybindings

## Scripting

-   [ ] Use sol:
    -   Tutorial: https://sol2.readthedocs.io/en/latest/tutorial/all-the-things.html
    -   GitHub: https://github.com/ThePhD/sol2

## Map

-   [ ] Investigate whether the map can be stored more efficiently. Place to start: [quadtree](https://en.wikipedia.org/wiki/Quadtree).
