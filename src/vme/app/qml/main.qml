import app
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import AppComponents as VMEComponent

Item {
    id: root;
    width: 1200;
    height: 800;

    property var editor;
    property var tilesetModel;

    DropArea {
        id: baseDropThing;
        anchors.fill: parent;

        onDropped: (drop) => {
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

        onEntered: (drag) => {
            // console.log("Base::Entered");
        }

        onExited: (drag)=> {
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
        anchors.fill: parent;

        MenuBar {
            Layout.fillWidth: true

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
        } // Top menu

        Rectangle {
            id: contentRoot;

            Layout.fillWidth: true;
            Layout.fillHeight: true;

            GridLayout {
                anchors.fill: parent;

                rowSpacing: 0;
                columnSpacing: 0

                rows: 3;
                columns: 3;

                
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

                                Layout.fillWidth: true
                                Layout.fillHeight: true
                            }
                        }
                    }
                }

                // VMEComponent.ThingGrid {
                //     id: g

                //     Layout.fillHeight: true;
                //     Layout.row: 1;
                //     Layout.column: 0;
                // }

                VMEComponent.VerticalPanelArea {
                    Layout.fillHeight: true;
                    Layout.row: 1;
                    Layout.column: 0;
                    Layout.minimumWidth: 200;
                }
                
                // VMEComponent.ItemPalette {
                //     color: "black";
                //     model: root.tilesetModel;

                //     Layout.preferredWidth: rectWidth;
                //     Layout.fillHeight: true;
                //     Layout.row: 1;
                //     Layout.column: 0;
                // }

                // Rectangle {
                //     color: "green"
                //     Layout.preferredHeight: 80
                //     Layout.fillWidth: true
                //     Layout.columnSpan: 3
                //     Layout.row: 0
                //     Layout.column: 0
                // }


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
            } // Main grid


            Repeater {
                model: 6

                VMEComponent.ItemPanel {
                    x: Math.random() * (1200 / 2 - 100);
                    y: Math.random() * (800 - 100);

                    color: "black";

                    VMEComponent.ThingList {
                        Layout.fillWidth: true;
                        Layout.fillHeight: true;
                        
                        model: {
                            root.tilesetModel;
                        }
                    }
                }

                // VMEComponent.ItemPanel {
                //     x: Math.random() * (1200 / 2 - 100);
                //     y: Math.random() * (800 - 100);

                //     color: Qt.rgba(Math.random(), Math.random(), Math.random(), 1);

                //     border {
                //         width: 2;
                //         color: "white"
                //     }

                //     Text {
                //         anchors.centerIn: parent
                //         text: index
                //         color: "white"
                //     }
                // }

                // VMEComponent.ResizableItem {
                //     id: rect;
                    
                //     z: mouseArea.drag.active || mouseArea.pressed ? 2 : 1;
                //     color: Qt.rgba(Math.random(), Math.random(), Math.random(), 1);
                //     x: Math.random() * (1200 / 2 - 100);
                //     y: Math.random() * (800 - 100);

                //     property var trueParent;

                //     // property point beginDrag
                //     property bool caught: false

                //     border {
                //         width: 2;
                //         color: "white"
                //     }

                //     radius: 5
                //     Drag.active: mouseArea.drag.active
                //     Drag.supportedActions: Qt.MoveAction
                //     Drag.proposedAction: Qt.MoveAction

                //     property var onDetach: null;


                //     Text {
                //         anchors.centerIn: parent
                //         text: index
                //         color: "white"
                //     }

                //     // DropArea {

                //     // }


                //     MouseArea {
                //         id: mouseArea
                //         property bool dragActive: drag.active

                //         anchors.fill: parent

                //         drag.target: parent

                //         onDragActiveChanged: () => {
                //             if(drag.active) {
                //                 console.log("Drag.onDragStarted");
                //                 // if (rect.parent.beforeRemoveItem) {
                //                 //     rect.parent.beforeRemoveItem(rect);
                //                 // }

                //                 if (rect.onDetach) {
                //                     rect.onDetach();
                //                     rect.onDetach = null;
                //                 }

                //                 rect.trueParent = rect.parent;
                //                 rect.parent = contentRoot;
                //             }
                //         }
                //         onPressed: {
                //             // rect.beginDrag = Qt.point(rect.x, rect.y);
                //         }
                //         onReleased: {
                //             const result = parent.Drag.drop();
                //             // if (result == Qt.MoveAction) {
                //             // } else {
                //             //     backAnimX.from = rect.x;
                //             //     backAnimX.to = beginDrag.x;
                //             //     backAnimY.from = rect.y;
                //             //     backAnimY.to = beginDrag.y;
                //             //     backAnim.start()
                //             // }
                //         }
                //     }

                //     // ParallelAnimation {
                //     //     id: backAnim
                //     //     SpringAnimation { id: backAnimX; target: rect; property: "x"; duration: 500; spring: 2; damping: 0.2 }
                //     //     SpringAnimation { id: backAnimY; target: rect; property: "y"; duration: 500; spring: 2; damping: 0.2 }
                //     // }
                // }
            }

        } // contentRoot
    }
}
