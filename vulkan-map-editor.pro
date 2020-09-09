QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17 console

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

win32:INCLUDEPATH += D:\Programs\glm-0.9.9.8\glm
# win32:INCLUDEPATH += "C:\Users\giuin\AppData\Local\vcpkg\vcpkg\installed\x64-windows\include"


# CONFIG(debug) {
#
#     win32:LIBS += -L"C:/Users/wrose/Documents/development/vcpkg/packages/boost-filesystem_x64-windows/debug/lib"
#     win32:LIBS += -L"C:/Users/wrose/Documents/development/vcpkg/packages/boost-system_x64-windows/debug/lib"
#     win32:LIBS += -lboost_filesystem-vc140-mt-gd-1_66 -lboost_system-vc140-mt-gd-1_66
#     message("Debug Config")
# } else {
#     message("Release Config")
# }

SOURCES += \
    main.cpp \
    gui/mainwindow.cpp \
    gui/vulkan_window.cpp \
    gui/item_list.cpp \
    gui/border_layout.cpp \
    graphics/appearances.cpp \
    graphics/compression.cpp \ # graphics/resource-descriptor.cpp \
    graphics/texture_atlas.cpp \
    graphics/vulkan_helpers.cpp \
    graphics/batch_item_draw.cpp \ # graphics/device_manager.cpp \ # graphics/swapchain.cpp \
    graphics/buffer.cpp \ # graphics/engine.cpp \
    graphics/texture.cpp \ # graphics/vulkan_debug.cpp \
    graphics/protobuf/appearances.pb.cc \
    graphics/protobuf/map.pb.cc \
    graphics/protobuf/shared.pb.cc \
    position.cpp \
    util.cpp \
    file.cpp \
    time_point.cpp \
    logger.cpp \
    camera.cpp \
    random.cpp \
    map_renderer.cpp \
    item.cpp \
    items.cpp \
    map.cpp \
    map_view.cpp \
    ecs/ecs.cpp \
    ecs/item_animation.cpp \
    item_attribute.cpp \
    otb.cpp \
    quad_tree.cpp \
    tile_location.cpp \
    tile.cpp \
    item_type.cpp \
    map_io.cpp \
    town.cpp \
    action/action.cpp \
    selection.cpp \
    qt/logging.cpp \

# LZMA
SOURCES += \
    lzma/Alloc.c \
    lzma/LzFind.c \
    lzma/LzFindMt.c \
    lzma/LzmaDec.c \
    lzma/LzmaEnc.c \
    lzma/LzmaLib.c \
    lzma/Threads.c \

HEADERS += \
    lzma/7zTypes.h \
    lzma/Alloc.h \
    lzma/Compiler.h \
    lzma/LzFind.h \
    lzma/LzFindMt.h \
    lzma/LzHash.h \
    lzma/LzmaDec.h \
    lzma/LzmaEnc.h \
    lzma/LzmaLib.h \
    lzma/Precomp.h \
    lzma/Threads.h \
# End LZMA

HEADERS += \
    main.h \
    gui/mainwindow.h \
    gui/vulkan_window.h \ # graphics/vulkan_debug.h \
    gui/item_list.h \
    gui/border_layout.h \
    graphics/appearances.h \
    graphics/compression.h \ # graphics/resource-descriptor.h \
    graphics/texture_atlas.h \
    graphics/vulkan_helpers.h \
    graphics/batch_item_draw.h \ # graphics/device_manager.h \
    graphics/validation.h \
    graphics/vertex.h \ # graphics/swapchain.h \
    graphics/buffer.h \ #graphics/engine.h \
    graphics/texture.h \
    graphics/protobuf/appearances.pb.h \
    graphics/protobuf/map.pb.h \
    graphics/protobuf/shared.pb.h \
    position.h \
    const.h \
    debug.h \
    util.h \
    file.h \
    time_point.h \
    logger.h \
    camera.h \
    random.h \
    map_renderer.h \
    item.h \
    items.h \
    map.h \
    map_view.h \
    ecs/ecs.h \
    ecs/entity.h \
    ecs/component_array.h \
    ecs/item_animation.h \
    item_attribute.h \
    otb.h \
    quad_tree.h \
    tile_location.h \
    tile.h \
    item_type.h \
    map_io.h \
    town.h \
    type_trait.h \
    version.h \
    action/action.h \
    selection.h \
    definitions.h \
    qt/logging.h \
    qt/qt_util.h


FORMS += \
    gui/mainwindow.ui
    gui/vulkan_window.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

# Google protobuf
INCLUDEPATH += "C:/Users/giuin/AppData/Local/vcpkg/vcpkg/packages/protobuf_x64-windows/include"
DEPENDPATH += "C:/Users/giuin/AppData/Local/vcpkg/vcpkg/packages/protobuf_x64-windows/include"

CONFIG(debug, debug|release) {
    win32:LIBS += "C:/Users/giuin/AppData/Local/vcpkg/vcpkg/packages/protobuf_x64-windows/debug/lib/libprotobufd.lib"
    win32:LIBS += "C:/Users/giuin/AppData/Local/vcpkg/vcpkg/packages/protobuf_x64-windows/debug/lib/libprotobuf-lited.lib"

 }

 CONFIG(release, debug|release) {
    win32:LIBS += "C:/Users/giuin/AppData/Local/vcpkg/vcpkg/packages/protobuf_x64-windows/lib/libprotobuf.lib"
    win32:LIBS += "C:/Users/giuin/AppData/Local/vcpkg/vcpkg/packages/protobuf_x64-windows/lib/libprotobuf-lite.lib"
 }


#liblzma
win32: LIBS += -LC:/Users/giuin/AppData/Local/vcpkg/vcpkg/packages/liblzma_x64-windows/lib/ -llzma
INCLUDEPATH += C:/Users/giuin/AppData/Local/vcpkg/vcpkg/packages/liblzma_x64-windows/include
DEPENDPATH += C:/Users/giuin/AppData/Local/vcpkg/vcpkg/packages/liblzma_x64-windows/include

# stbi
INCLUDEPATH += C:/Users/giuin/AppData/Local/vcpkg/vcpkg/packages/stb_x64-windows/include

# pugixml
win32: LIBS += -LC:/Users/giuin/AppData/Local/vcpkg/vcpkg/packages/pugixml_x64-windows/lib/ -lpugixml
INCLUDEPATH += C:/Users/giuin/AppData/Local/vcpkg/vcpkg/packages/pugixml_x64-windows/include
DEPENDPATH += C:/Users/giuin/AppData/Local/vcpkg/vcpkg/packages/pugixml_x64-windows/include

# nlohmann-json
INCLUDEPATH += C:/Users/giuin/AppData/Local/vcpkg/vcpkg/packages/nlohmann-json_x64-windows/include