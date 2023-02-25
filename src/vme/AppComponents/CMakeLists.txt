cmake_minimum_required(VERSION 3.16)

set(CMAKE_PREFIX_PATH "C:/Qt/6.3.1/msvc2019_64/lib/cmake")

set(CMAKE_AUTOMOC ON)


find_package(Qt6 REQUIRED COMPONENTS Core Qml Quick)

set(HEADER_FILES
    gui_thing_image.h
    ui_resources.h
    tileset_model.h
    enum_conversion.h
    qml_ui_utils.h
    qt_vulkan_info.h
    item_palette_store.h
)

set(SRC_FILES
    gui_thing_image.cpp
    ui_resources.cpp
    tileset_model.cpp
    enum_conversion.cpp
    qml_ui_utils.cpp
    qt_vulkan_info.cpp
    item_palette_store.cpp
)

qt_add_library(app_components STATIC
    ${HEADER_FILES}
    ${SRC_FILES}
)

target_include_directories(app_components INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})


target_link_libraries(app_components PRIVATE core app_componentsplugin Qt::Gui Qt::Quick Qt::QuickControls2)


set(QML_HEADER_FILES
    qml_map_item.h
)


set(QML_SRC_FILES
    qml_map_item.cpp
)


qt_add_qml_module(app_components
    URI AppComponents
    VERSION 1.0
    SOURCES
        ${QML_HEADER_FILES} ${QML_SRC_FILES}
    QML_FILES
        ResizableItem.qml
        ThingList.qml
        ItemPalette.qml
        MapView.qml
        # ThingTile.qml
        # ThingGrid.qml
) 