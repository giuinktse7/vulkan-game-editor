cmake_minimum_required(VERSION 3.16)

# set(CMAKE_PREFIX_PATH "C:/Qt/6.3.1/msvc2019_64/lib/cmake")

set(CMAKE_AUTOMOC ON)


find_package(Qt6 REQUIRED COMPONENTS Core Qml Quick)

# https://doc.qt.io/qt-6/qt-cmake-policy-qtp0001.html
if(QT_KNOWN_POLICY_QTP0001)
    qt_policy(SET QTP0001 NEW)
endif()

set(HEADER_FILES
    gui_thing_image.h
    ui_resources.h
    tileset_model.h
    combobox_model.h
    context_menu_model.h
    enum_conversion.h
    qml_ui_utils.h
    qt_vulkan_info.h
    town_list_model.h
    search_result_model.h
    minimap.h
)

set(SRC_FILES
    gui_thing_image.cpp
    ui_resources.cpp
    tileset_model.cpp
    combobox_model.cpp
    context_menu_model.cpp
    enum_conversion.cpp
    qml_ui_utils.cpp
    qt_vulkan_info.cpp
    town_list_model.cpp
    search_result_model.cpp
    minimap.cpp
)

qt_add_library(app_components STATIC
    ${HEADER_FILES}
    ${SRC_FILES}
)

target_include_directories(app_components INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})


target_link_libraries(app_components PRIVATE core app_componentsplugin Qt::Gui Qt::Quick Qt::QuickControls2)


set(QML_HEADER_FILES
    qml_map_item.h
    refreshable_image.h
)


set(QML_SRC_FILES
    qml_map_item.cpp
    refreshable_image.cpp
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
        TownWindow.qml
        InputField.qml
        BorderedLayout.qml
        MapView.qml
        MapTabButton.qml
        VerticalPanelArea.qml
        ItemPanel.qml
        SearchPopupView.qml
        SearchPopupFilterChoice.qml
        MinimapView.qml
        # ThingTile.qml
        # ThingGrid.qml
    RESOURCES
        cross.svg
)


