# TODO

## Rendering

## Map Rendering

- [ ] Render creature outfits by creating a copy of the relevant atlases and multiplying in pixel colors.
  - Yellow: head
  - Red: body
  - Green: legs
  - Blue: feet
  When requesting a sprite for a CreatureType:
    - The CreatureType makes copy of its required texture atlases and updates color of all(to start with?) its outfits based on their color templates.
    - TextureAtlas has template instantiations, either in a static TextureAtlas::instatiations `map (id -> std::vector<pair<lookHash, Texture>>)` or stored as a variable in a TextureAtlas.
    - Or maybe each atlas stores its "default instantiation" as the first item in a vector of `std::pair<LookHash, Texture>`. Then you need a TextureAtlas and the
    LookHash to get a texture from the atlas. In default case, we always take the first texture (default instantiation).
    - Must also handle the case where the creature has the look of an item
- [x] ~~Implement animation~~
- [x] ~~Handle stackable appearances~~
- [x] ~~Handle appearances with elevation~~
- [ ] Feature?: Maybe clicking on a selected item should deselect it. If whole tile, the whole tile should maybe be deselected?
- [x] ~~Feature?: When removing in area (Drag while Ctrl+Shift), show selection rectangle (red tinted?) with the selected item at mouse cursor ONLY.~~
- [x] ~~Feature: Instant feedback for what items will be removed when removing in area (Drag while Ctrl+Shift).~~
- [ ] Animations

      - [ ] Implement rendering for animations through MapView::requestUpdate().
      - [x] Only update visible animations.
      - [ ] Do not render animations when zoomed out further than a certain level.

## Editing functionality

- [ ] BUG:

            1. Open bag with bag B in first slot.
            2. Move second item X to first slot.
            3. place X into B.
            4. Undo.

      Crashes because when (2) is committed, it uses index=1 for B instead of index=0.

- [ ] topItem selected: RELEASE deselects, but ONLY if mouse has been outside of this tile since the PRESS event.
- [ ] Reorder items on a tile.
- [ ] (_Stretch_): reorder an item group among selected tiles. For example, move grass borders one level up.
- [x] ~~Move selected items~~
- [x] ~~topItem **not** selected: PRESS selects.~~
- [x] ~~BUG: A selection can be moved out of bounds. It should clamp to the map size, [and maybe to max/min floor?]~~
      This would require the editor to know about different item groups, like what items are grass borders.

## Optimization

- [ ] Optimize Move actions of large areas by moving map nodes instead of individual tiles.

### GUI

- [ ]
  BUG: Minimize and then returning to the window (maximizing) does not retain
  the correct window/widget focus.
- [x] ~~Handle map keyboard events through the scrollbar widget~~
- [ ] Draggable split views. [QSplitter](https://doc.qt.io/qt-5/qsplitter.html#details).
      Subclass [QTabBar](https://doc.qt.io/qt-5/qtabbar.html) to implement dragging of tabs. [Drag and drop](https://doc.qt.io/qt-5/dnd.html).

      **Note**: Might possibly have to enable drag events on source and/or target widget. See [this](https://forum.qt.io/topic/67542/drag-tabs-between-qtabwidgets/4).

## General

- [ ] System for custom keybindings

## Map

- [ ] Investigate whether the map can be stored more efficiently. Place to start: [quadtree](https://en.wikipedia.org/wiki/Quadtree).
