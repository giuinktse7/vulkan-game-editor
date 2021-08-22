# TODO

## Palettes

- tilesets.json
  - Continue grounds at 13585 in RME (13145 is done)

## Crashes

## Rendering

## Map Rendering

- [ ] Add possbility for a CreatureType to have the look of an item
- [x] ~~Implement animation~~
- [x] ~~Handle stackable appearances~~
- [x] ~~Handle appearances with elevation~~
- [x] ~~Feature?: When removing in area (Drag while Ctrl+Shift), show selection rectangle (red tinted?) with the selected item at mouse cursor ONLY.~~
- [x] ~~Feature: Instant feedback for what items will be removed when removing in area (Drag while Ctrl+Shift).~~
- [ ] Animations
  - Implementation:
    - When rendering map, store the lowest animation delay D for next animation for any rendered item.
    - If any animation was found, queue another render with delay D. Otherwise, no render queueing is necessary.
  - [x] Only update visible animations.
  - [ ] Do not render animations when zoomed out further than a certain level.
- [ ] Light effects (For example from torches)

## Editing functionality

- [ ] topItem selected: RELEASE deselects, but ONLY if mouse has been outside of this tile since the PRESS event.
- [ ] Reorder items on a tile.
- [ ] (_Stretch_): reorder an item group among selected tiles. For example, move grass borders one level up.
- [x] ~~Move selected items~~
- [x] ~~topItem **not** selected: PRESS selects.~~
- [x] ~~BUG: A selection can be moved out of bounds. It should clamp to the map size, [and maybe to max/min floor?]~~

## Optimization

### GUI

- [ ] Draggable split views. [QSplitter](https://doc.qt.io/qt-5/qsplitter.html#details).
  Subclass [QTabBar](https://doc.qt.io/qt-5/qtabbar.html) to implement dragging of tabs. [Drag and drop](https://doc.qt.io/qt-5/dnd.html).

  **Note**: Might possibly have to enable drag events on source and/or target widget. See [this](https://forum.qt.io/topic/67542/drag-tabs-between-qtabwidgets/4).

## General

- [ ] System for customizable keybindings

## Map

- [ ] Investigate whether the map can be stored more efficiently. Place to start: [quadtree](https://en.wikipedia.org/wiki/Quadtree).
