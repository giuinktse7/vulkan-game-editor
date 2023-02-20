import app
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import AppComponents as VMEComponent

Item {
    id: root;
    width: 800;
    height: 600;

    property var editor;
    property var tilesetModel;


    // MapViewItem {
    //     anchors.fill: parent
    //     focus: true
    
    //     Keys.onPressed: (event) => { console.log("pressed " + event.key); }
    // }


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

   GridLayout {
        id: grid;
        anchors.fill: parent;
        rowSpacing: 0;
        columnSpacing: 0

        rows: 3;
        columns: 3;

        VMEComponent.ItemPalette {
            color: "blue";
            model: root.tilesetModel;

            Layout.preferredWidth: rectWidth;
            Layout.fillHeight: true;
            Layout.row: 1;
            Layout.column: 0;
        }

        MenuBar {
            Layout.fillWidth: true
            Layout.columnSpan: 3
            Layout.row: 0
            Layout.column: 0

            Menu {
                title: qsTr("&File")
                Action {
                    text: qsTr("&New...");
                    shortcut: "Ctrl+N";
                }
                Action { 
                    text: qsTr("&Open...") 
                    shortcut: "Ctrl+O";
                }
                Action { 
                    text: qsTr("&Save") 
                    shortcut: "Ctrl+S";
                }
                Action { 
                    text: qsTr("Save &As...") 
                    shortcut: "Ctrl+Alt+S";
                }
                Action { 
                    text: qsTr("Generate Map")
                }
                Action { 
                    text: qsTr("Close")
                    shortcut: "Ctrl+Q";
                }
                MenuSeparator { }
                Menu {
                    title: qsTr("Import")
                    Action { text: qsTr("Import Map...") }
                    Action { text: qsTr("Import Monsters/NPCs...") }
                    Action { text: qsTr("Import Minimap...") }
                }
                Menu {
                    title: qsTr("Export")
                    Action { text: qsTr("Export Minimap...") }
                }
                Action {
                    text: qsTr("Reload")
                    shortcut: "F5";
                }

                MenuSeparator { }
                Menu {
                    title: qsTr("Recent files")
                }
                Action { text: qsTr("Preferences") }
                Action { text: qsTr("&Exit") }
            }

            Menu {
                title: qsTr("&Edit")
                Action { text: qsTr("Undo") }
                Action { text: qsTr("Redo") }
                Action { text: qsTr("Find item...") }
                Action { text: qsTr("Replace item...") }
                Menu {
                    title: qsTr("Find on Map")
                    Action { text: qsTr("Find Everything") }
                    MenuSeparator { }
                    Action { text: qsTr("Find Unique") }
                    Action { text: qsTr("Find Action") }
                    Action { text: qsTr("Find Container") }
                    Action { text: qsTr("Find Writable") }
                }
                Menu {
                    title: qsTr("Find on Selected Area")
                    Action { text: qsTr("Find Everything") }

                    MenuSeparator { }

                    Action { text: qsTr("Find Unique") }
                    Action { text: qsTr("Find Action") }
                    Action { text: qsTr("Find Container") }
                    Action { text: qsTr("Find Writable") }

                    MenuSeparator { }
                }

                MenuSeparator { }
                
                Action { text: qsTr("Jump to brush...") }
                Action { text: qsTr("Jump to item...") }

                MenuSeparator { }
                
                Action {
                    text: qsTr("Cut");
                    shortcut: "Ctrl+X";
                    onTriggered: root.editor.cut();
                }
                Action {
                    text: qsTr("Copy");
                    shortcut: "Ctrl+C";
                    onTriggered: root.editor.copy();
                }
                Action {
                    text: qsTr("Paste");
                    shortcut: "Ctrl+V";
                    onTriggered: root.editor.paste();
                }

            }
           
            Menu {
                title: qsTr("&Help")
                Action { text: qsTr("&About") }
            }
        }
 
        // Rectangle {
        //     color: "green"
        //     Layout.preferredHeight: 80
        //     Layout.fillWidth: true
        //     Layout.columnSpan: 3
        //     Layout.row: 0
        //     Layout.column: 0
        // }

        ColumnLayout {
            Layout.row: 1
            Layout.column: 1
            Layout.alignment: Qt.AlignTop
            Layout.fillWidth: true
            Layout.fillHeight: true

            TabBar {
                id: tabRoot

                onCurrentIndexChanged: () => {
                    root.editor.currentMapIndex = currentIndex;
                }

                Repeater {
                    model: root.editor.mapTabs
                    delegate: TabButton {
                        horizontalPadding: 8;
                        verticalPadding: 4;
                        text: item
                    }
                }
            }

            StackLayout {
                id: mapAreaStackLayout

                Layout.fillWidth: true
                Layout.fillHeight: true

                currentIndex: tabRoot.currentIndex
                property var prevIndex: 0
                
                onCurrentIndexChanged: (x) => {
                    if (prevIndex != currentIndex) {
                        root.editor.mapTabSelected(prevIndex, currentIndex);
                    }

                    prevIndex = currentIndex;
                }

                Repeater {
                    model: root.editor.mapTabs
                    delegate: VMEComponent.MapView {
                        id: mapView

                        required property string item
                        required property int theId
                        required property int index

                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        Component.onCompleted: () => {
                            mapView.entryId = theId;
                        }
                    }
                }
            }
        }

        // }


        Rectangle {
            color: "purple"
            Layout.fillWidth: true
            Layout.preferredHeight: 30;
            Layout.columnSpan: 3
            Layout.row: 2
            Layout.column: 0
       }

        Rectangle {
            color: "green"
            Layout.fillHeight: true
            Layout.preferredWidth: 30;
            Layout.row: 1
            Layout.column: 2
       }
   }
}
