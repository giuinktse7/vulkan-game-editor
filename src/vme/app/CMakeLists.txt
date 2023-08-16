set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Enable Vulkan debugging layer
set(QSG_RHI_DEBUG_LAYER 1)

set(HEADER_FILES
    main.h
    debug_util.h
    qt_logging.h
    app_data_model.h
)

set(SRC_FILES
    main.cpp
    debug_util.cpp
    qt_logging.cpp
    app_data_model.cpp
)

# https://doc.qt.io/qt-6/qt-cmake-policy-qtp0001.html
if(QT_KNOWN_POLICY_QTP0001)
    qt_policy(SET QTP0001 NEW)
endif()


add_executable(app ${HEADER_FILES} ${SRC_FILES})

target_link_libraries(app PRIVATE core app_components app_componentsplugin Qt::Gui Qt::Quick Qt::QuickControls2)

qt_add_qml_module(app
    URI app
    VERSION 1.0
    QML_FILES
        qml/main.qml
)

set_target_properties(app PROPERTIES
    # Enable this to get rid of the console window
    # https://stackoverflow.com/questions/54413292/building-non-console-windows-app-with-qt-and-cmake
    # WIN32_EXECUTABLE ON
    
    MACOSX_BUNDLE ON
)