# TODO

## Rendering

## Map Rendering

- [x] ~~Implement animation~~
- [x] ~~Handle stackable appearances~~
- [x] ~~Handle appearances with elevation~~
- [ ] Feature?: Maybe clicking on a selected item should deselect it. If whole tile, the whole tile should maybe be deselected?
- [ ] Feature?: When removing in area (Drag while Ctrl+Shift), show selection rectangle (red tinted?) with the selected item at mouse cursor ONLY.
- [x] ~~Feature: Instant feedback for what items will be removed when removing in area (Drag while Ctrl+Shift).~~
- [ ] Animations

      - [ ] Implement rendering for animations through MapView::requestUpdate().
      - [ ] Only update visible animations.
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
