# TODO

## Rendering

## Map Rendering

- [x] Implement animation
- [ ] Handle stackable appearances
- [x] Handle appearances with elevation
- [ ] Feature?: Maybe clicking on a selected item should deselect it. If whole tile, the whole tile should maybe be deselected?
- [ ] Feature?: When removing in area (Drag while Ctrl+Shift), show selection rectangle (red tinted?) with the selected item at mouse cursor ONLY.
- [ ] Feature: Instant feedback for what items will be removed when removing in area (Drag while Ctrl+Shift).

## Editing functionality

- [ ] Move selected items
- [ ] topItem **not** selected: PRESS selects.
- [ ] topItem selected: RELEASE deselects, but ONLY if mouse has been outside of this tile since the PRESS event.
- [ ] BUG: A selection can be moved out of bounds. It should clamp to the map size, [and maybe to max/min floor?]
- [ ] Reorder items on a tile.
- [ ] (_Stretch_): reorder an item group among selected tiles. For example, move grass borders one level up.
      This would require the editor to know about different item groups, like what items are grass borders.

## Optimization

- [ ] Optimize Move actions of large areas by moving map nodes instead of individual tiles.

### GUI

- [ ]
  BUG: Minimize and then returning to the window (maximizing) does not retain
  the correct window/widget focus.
- [ ] Handle map keyboard events through the scrollbar widget
- [ ] Draggable split views. [QSplitter](https://doc.qt.io/qt-5/qsplitter.html#details).
      Subclass [QTabBar](https://doc.qt.io/qt-5/qtabbar.html) to implement dragging of tabs. [Drag and drop](https://doc.qt.io/qt-5/dnd.html).

      **Note**: Might possibly have to enable drag events on source and/or target widget. See [this](https://forum.qt.io/topic/67542/drag-tabs-between-qtabwidgets/4).

## General

- [ ] System for custom keybindings

## Map

- [ ] Investigate whether the map can be stored more efficiently. Place to start: [quadtree](https://en.wikipedia.org/wiki/Quadtree).
