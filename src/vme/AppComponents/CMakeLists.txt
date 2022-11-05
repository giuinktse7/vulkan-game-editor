cmake_minimum_required(VERSION 3.16)

set(CMAKE_PREFIX_PATH "C:/Qt/6.3.1/msvc2019_64/lib/cmake")

set(CMAKE_AUTOMOC ON)


find_package(Qt6 REQUIRED COMPONENTS Core Qml Quick)

qt_add_library(app_components STATIC)

set(HEADER_FILES
    my_type.h
    thing_list_model.h
)

set(SOURCE_FILES
    my_type.cpp
    thing_list_model.cpp
)



qt_add_qml_module(app_components
    URI AppComponents
    VERSION 1.0
    SOURCES
        ${HEADER_FILES} ${SOURCE_FILES}
    QML_FILES
        RedSquare.qml
        ThingList.qml
) 