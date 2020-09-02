# Vulkan Game Editor

## Terminology

### Action

An action is an event that can occur and can be undone/redone.

## Compiling

### Software

[QT5 and QT Creator](https://www.qt.io/download-open-source)

### Libraries

```bash
vcpkg install liblzma protobuf nlohmann-json stb pugixml libarchive
```

### Update library paths

Update the library paths in `vulkan-map-editor.pro`.

```qmake
# Google protobuf
INCLUDEPATH += ".../vcpkg/vcpkg/packages/protobuf_x64-windows/include"
DEPENDPATH += ".../vcpkg/vcpkg/packages/protobuf_x64-windows/include"

CONFIG(debug, debug|release) {
    win32:LIBS += ".../vcpkg/vcpkg/packages/protobuf_x64-windows/debug/lib/libprotobufd.lib"
    win32:LIBS += ".../vcpkg/vcpkg/packages/protobuf_x64-windows/debug/lib/libprotobuf-lited.lib"

 }

 CONFIG(release, debug|release) {
    win32:LIBS += ".../vcpkg/vcpkg/packages/protobuf_x64-windows/lib/libprotobuf.lib"
    win32:LIBS += ".../vcpkg/vcpkg/packages/protobuf_x64-windows/lib/libprotobuf-lite.lib"
 }


#liblzma
win32: LIBS += -L.../vcpkg/vcpkg/packages/liblzma_x64-windows/lib/ -llzma
INCLUDEPATH += .../vcpkg/vcpkg/packages/liblzma_x64-windows/include
DEPENDPATH += .../vcpkg/vcpkg/packages/liblzma_x64-windows/include

# stbi
INCLUDEPATH += .../vcpkg/vcpkg/packages/stb_x64-windows/include

# pugixml
win32: LIBS += -L.../vcpkg/vcpkg/packages/pugixml_x64-windows/lib/ -lpugixml
INCLUDEPATH += .../vcpkg/vcpkg/packages/pugixml_x64-windows/include
DEPENDPATH += .../vcpkg/vcpkg/packages/pugixml_x64-windows/include

# nlohmann-json
INCLUDEPATH += .../vcpkg/vcpkg/packages/nlohmann-json_x64-windows/include
```
