import app
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import AppComponents as VMEComponent
import QtQuick.Dialogs
import QtCore
import VME.dataModel 1.0

Item {
    id: root
    width: 1200
    height: 800

    function createItemPalette(parent) {
        const component = Qt.createComponent(Qt.url("../../AppComponents/ItemPalette.qml"));
        if (component.status == Component.Ready) {
            return component.createObject(parent);
        } else {
            console.log("Error loading component:", component.errorString());
            component.statusChanged.connect(() => {
                    if (component.status == Component.Ready) {
                        return component.createObject(parent);
                    } else {
                        console.log("Error loading component:", component.errorString());
                    }
                });
        }
    }

    FileDialog {
        id: saveMapFileDialog
        title: "Save Map"
        currentFolder: StandardPaths.standardLocations(StandardPaths.DocumentsLocation)[0]
        fileMode: FileDialog.SaveFile
        nameFilters: ["OTBM files (*.otbm)"]
        onAccepted: () => {
            AppDataModel.saveCurrentMap(selectedFile);
        }
    }

    FileDialog {
        id: loadMapFileDialog
        title: "Load Map"
        currentFolder: StandardPaths.standardLocations(StandardPaths.DocumentsLocation)[0]
        fileMode: FileDialog.OpenFile
        nameFilters: ["OTBM files (*.otbm)"]
        onAccepted: () => {
            AppDataModel.loadMap(selectedFile);
        }
    }

    Connections {
        target: AppDataModel

        /**
        * Listens to the openFolderDialog signal from the AppDataModel C++ class
        */
        function onOpenFolderDialog(dialog_id) {
            if (dialog_id === "save_map_location") {
                saveMapFileDialog.open();
            }
        }
    }

    DropArea {
        id: baseDropThing
        anchors.fill: parent

        onDropped: drop => {
            // console.log("Base::Dropped");
            if (drop) {
                drop.acceptProposedAction();
                let point = drop.source.parent.mapToGlobal(drop.source.x, drop.source.y);
                point = parent.mapFromGlobal(point.x, point.y);
                drop.source.parent = parent;
                drop.source.x = point.x;
                drop.source.y = point.y;
            }
        }

        onEntered: drag => {}

        onExited: drag => {
            // console.log("Base::Exited");
            if (drag) {
                drag.source.caught = false;
            }
        }
    }

    // Rectangle {
    //     color: "transparent"
    //     anchors.fill: parent

    //     ColumnLayout {
    //         spacing: 0

    //         Rectangle {
    //             Layout.fillWidth: true
    //             color: "red"
    //             Layout.preferredWidth: 40
    //             Layout.preferredHeight: 40
    //         }

    //         Rectangle {
    //             Layout.fillWidth: true
    //             color: "green"
    //             Layout.preferredWidth: 40
    //             Layout.preferredHeight: 70
    //         }

    //         Rectangle {
    //             Layout.fillWidth: true
    //             Layout.fillHeight: true
    //             color: "blue"
    //             Layout.preferredWidth: 70
    //             Layout.preferredHeight: 40
    //         }
    //     }
    // }
    ColumnLayout {
        anchors.fill: parent

        MenuBar {
            Layout.fillWidth: true

            Menu {
                title: qsTr("&File")
                Action {
                    text: qsTr("&New...")
                    shortcut: "Ctrl+N"
                }
                Action {
                    text: qsTr("&Open...")
                    shortcut: "Ctrl+O"
                    onTriggered: {
                        loadMapFileDialog.open();
                    }
                }
                Action {
                    text: qsTr("&Save")
                    shortcut: "Ctrl+S"
                    onTriggered: AppDataModel.saveMap()
                }
                Action {
                    text: qsTr("Save &As...")
                    shortcut: "Ctrl+Alt+S"
                }
                Action {
                    text: qsTr("Generate Map")
                }
                Action {
                    text: qsTr("Close")
                    shortcut: "Ctrl+Q"
                }
                MenuSeparator {
                }
                Menu {
                    title: qsTr("Import")
                    Action {
                        text: qsTr("Import Map...")
                    }
                    Action {
                        text: qsTr("Import Monsters/NPCs...")
                    }
                    Action {
                        text: qsTr("Import Minimap...")
                    }
                }
                Menu {
                    title: qsTr("Export")
                    Action {
                        text: qsTr("Export Minimap...")
                    }
                }
                Action {
                    text: qsTr("Reload")
                    shortcut: "F5"
                }

                MenuSeparator {
                }
                Menu {
                    title: qsTr("Recent files")
                }
                Action {
                    text: qsTr("Preferences")
                }
                Action {
                    text: qsTr("&Exit")
                }
            }

            Menu {
                title: qsTr("&Edit")
                Action {
                    text: qsTr("Undo")
                }
                Action {
                    text: qsTr("Redo")
                }
                Action {
                    text: qsTr("Find item...")
                }
                Action {
                    text: qsTr("Replace item...")
                }
                Menu {
                    title: qsTr("Find on Map")
                    Action {
                        text: qsTr("Find Everything")
                    }
                    MenuSeparator {
                    }
                    Action {
                        text: qsTr("Find Unique")
                    }
                    Action {
                        text: qsTr("Find Action")
                    }
                    Action {
                        text: qsTr("Find Container")
                    }
                    Action {
                        text: qsTr("Find Writable")
                    }
                }
                Menu {
                    title: qsTr("Find on Selected Area")
                    Action {
                        text: qsTr("Find Everything")
                    }

                    MenuSeparator {
                    }

                    Action {
                        text: qsTr("Find Unique")
                    }
                    Action {
                        text: qsTr("Find Action")
                    }
                    Action {
                        text: qsTr("Find Container")
                    }
                    Action {
                        text: qsTr("Find Writable")
                    }

                    MenuSeparator {
                    }
                }

                MenuSeparator {
                }

                Action {
                    text: qsTr("Jump to brush...")
                }
                Action {
                    text: qsTr("Jump to item...")
                }

                MenuSeparator {
                }

                Action {
                    text: qsTr("Cut")
                    shortcut: "Ctrl+X"
                    onTriggered: AppDataModel.cut()
                }
                Action {
                    text: qsTr("Copy")
                    shortcut: "Ctrl+C"
                    onTriggered: AppDataModel.copy()
                }
                Action {
                    text: qsTr("Paste")
                    shortcut: "Ctrl+V"
                    onTriggered: AppDataModel.paste()
                }
            }

            Menu {
                title: qsTr("View")
                Action {
                    text: qsTr("New item palette")
                    onTriggered: () => {
                        createItemPalette(contentRoot);
                    }
                }
            }

            Menu {
                title: qsTr("&Help")
                Action {
                    text: qsTr("&About")
                }
            }
        } // Top menu

        Rectangle {
            id: contentRoot

            Layout.fillWidth: true
            Layout.fillHeight: true

            GridLayout {
                anchors.fill: parent

                rowSpacing: 0
                columnSpacing: 0

                rows: 3
                columns: 3

                ColumnLayout {
                    Layout.row: 1
                    Layout.column: 1
                    Layout.alignment: Qt.AlignTop
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    TabBar {
                        id: tabRoot

                        onCurrentIndexChanged: () => {
                            // Necessary because the tab bar will sometimes try to set the index to a value that is
                            // out of bounds. Unclear why.
                            if (currentIndex >= AppDataModel.mapTabs.size) {
                                currentIndex = AppDataModel.mapTabs.size - 1;
                                return;
                            }
                        }

                        Repeater {
                            model: AppDataModel.mapTabs
                            delegate: VMEComponent.MapTabButton {
                                horizontalPadding: 8
                                verticalPadding: 4
                                text: item
                                active: tabRoot.currentIndex == index

                                onClose: () => AppDataModel.closeMap(theId)
                            }
                        }
                    }

                    Binding {
                        target: AppDataModel
                        property: "currentMapIndex"
                        value: tabRoot.currentIndex
                    }

                    Binding {
                        target: tabRoot
                        property: "currentIndex"
                        value: AppDataModel.currentMapIndex
                    }

                    StackLayout {
                        id: mapAreaStackLayout

                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        currentIndex: tabRoot.currentIndex

                        Repeater {
                            model: AppDataModel.mapTabs
                            delegate: VMEComponent.MapView {
                                id: mapView

                                Layout.fillWidth: true
                                Layout.fillHeight: true
                            }
                        }
                    }
                }

                VMEComponent.VerticalPanelArea {
                    id: leftPanelArea
                    Layout.fillHeight: true
                    Layout.row: 1
                    Layout.column: 0
                    Layout.minimumWidth: 200

                    Component.onCompleted: () => {
                        const itemPalette = root.createItemPalette(leftPanelArea);
                        leftPanelArea.addItemToLayout(itemPalette);
                    }
                }

                Rectangle {
                    color: "purple"
                    Layout.fillWidth: true
                    Layout.preferredHeight: 30
                    Layout.columnSpan: 3
                    Layout.row: 2
                    Layout.column: 0
                }

                Rectangle {
                    color: "green"
                    Layout.fillHeight: true
                    Layout.preferredWidth: 30
                    Layout.row: 1
                    Layout.column: 2
                }
            } // Main grid
        } // contentRoot
    }
}
