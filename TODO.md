# TODO

## Rendering

- [x]
  Color mixes are too dark. Blend issue? Full colors look right (like 0xFFFF0000, 0xFFFF00FF, ...). But non-full colors do not (like 0xFFAA0000).
  Clear value does not seem to affect the blend issue.

## Map Rendering

- [x] Use own container for Appearance
- [x] Use object sprite id to get sprite texture
- [x] Use sprite patterns
- [x] Implement animation
- [ ] Handle stackable appearances
- [x] Handle appearances with elevation
- [ ] BUG: Ground (and bottom stackpos items) should not be affected by tile elevation when rendered (both normally and as item preview)
- [ ] BUG: ESC should clear selections
- [ ] Feature?: Maybe clicking on a selected item should deselect it. If whole tile, the whole tile should maybe be deselected?
- [x] Use mapView's mouseAction() to render the currently selected raw itemType.

## Editing functionality

- [x] Implement multi-select
- [x] Delete selected items
- [ ] Move selected items
- [ ] topItem **not** selected: PRESS selects.
- [ ] topItem selected: RELEASE deselects, but ONLY if mouse has been outside of this tile since the PRESS event.

### GUI

- [x] Replace Dear ImGui with QT
- [ ]
  BUG: Minimize and then returning to the window (maximizing) does not retain
  the correct window/widget focus.
- [x] Scroll the map using scroll bars
- [ ] Handle map keyboard events through the scrollbar widget

## General

- [x] Refactor the TimeMeasure class into TimePoint or similar
- [ ] System for custom keybindings
