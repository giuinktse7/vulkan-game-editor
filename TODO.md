# TODO

## Rendering

## Map Rendering

- [x] Implement animation
- [ ] Handle stackable appearances
- [x] Handle appearances with elevation
- [ ] BUG: Ground (and bottom stackpos items) should not be affected by tile elevation when rendered (both normally and as item preview)
- [ ] BUG: ESC should clear selections
- [ ] Feature?: Maybe clicking on a selected item should deselect it. If whole tile, the whole tile should maybe be deselected?

## Editing functionality

- [ ] Move selected items
- [ ] topItem **not** selected: PRESS selects.
- [ ] topItem selected: RELEASE deselects, but ONLY if mouse has been outside of this tile since the PRESS event.

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
