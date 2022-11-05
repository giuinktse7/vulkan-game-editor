cmake_minimum_required(VERSION 3.16)

set(CMAKE_PREFIX_PATH "C:/Qt/6.3.1/msvc2019_64/lib/cmake")

set(CMAKE_AUTOMOC ON)


find_package(Qt6 REQUIRED COMPONENTS Core Qml Quick)

set(HEADER_FILES
    gui_thing_image.h
)

set(SRC_FILES
    gui_thing_image.cpp
)

qt_add_library(app_components STATIC
    ${HEADER_FILES}
    ${SRC_FILES}
)

target_include_directories(app_components INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})


target_link_libraries(app_components PRIVATE core app_componentsplugin Qt::Gui Qt::Quick Qt::QuickControls2)


set(QML_HEADER_FILES
    my_type.h
    thing_list_model.h
)

set(QML_SRC_FILES
    my_type.cpp
    thing_list_model.cpp
)



qt_add_qml_module(app_components
    URI AppComponents
    VERSION 1.0
    SOURCES
        ${QML_HEADER_FILES} ${QML_SRC_FILES}
    QML_FILES
        RedSquare.qml
        ThingList.qml
) 