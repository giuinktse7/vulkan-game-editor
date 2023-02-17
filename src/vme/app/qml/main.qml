import app
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import AppComponents as VMEComponent

Item {
    id: root;
    width: 800;
    height: 600;

    property var tilesetModel;


    // MapViewItem {
    //     anchors.fill: parent
    //     focus: true
    
    //     Keys.onPressed: (event) => { console.log("pressed " + event.key); }
    // }

    QmlMapItem {
        id: renderer
        anchors.fill: parent
        anchors.margins: 0
        focus: true
        onActiveFocusChanged: console.log("uu")

        MouseArea {
            anchors.fill: parent;
            acceptedButtons: Qt.LeftButton | Qt.RightButton;
            propagateComposedEvents: true;

            onClicked: (mouse) => {
                mouse.accepted = false;

                if (mouse.button === Qt.RightButton) {
                    contextMenu.popup();
                } else if (mouse.button === Qt.LeftButton) {
                    renderer.forceActiveFocus();
                }
            }

            onPressed: (mouse) => {
                mouse.accepted = false;

                 if (mouse.button === Qt.RightButton) {
                    contextMenu.popup();
                } else if (mouse.button === Qt.LeftButton) {
                    renderer.forceActiveFocus();
                }
            }

            onPressAndHold: (mouse) => {
                mouse.accepted = false;

                if (mouse.source === Qt.MouseEventNotSynthesized)
                    contextMenu.popup()
            }

            Menu {
                id: contextMenu
                MenuItem { text: "Cut" }
                MenuItem { text: "Copy" }
                MenuItem { text: "Paste" }
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

   GridLayout {
       id: grid
       anchors.fill: parent
  
       rows: 3
       columns: 3
 
       Rectangle {
            color: "green"
            Layout.preferredHeight: 80
            Layout.fillWidth: true
            Layout.columnSpan: 3
            Layout.row: 0
            Layout.column: 0
       }
       
        Rectangle {
            color: "blue"
            property var rectWidth: 150;
            Layout.preferredWidth: rectWidth;
            Layout.fillHeight: true;
            Layout.row: 1;
            Layout.column: 0;

            VMEComponent.ThingList {
                model: {
                    root.tilesetModel;
                }
            }

            MouseArea {
                property var pressWidth;
                property var pressX;
                
                id: roiRectArea;
                width: 5;
                anchors.top: parent.top;
                anchors.bottom: parent.bottom;
                anchors.right: parent.right;
                hoverEnabled: true;
                cursorShape: containsMouse ? Qt.SizeHorCursor : Qt.ArrowCursor;
                acceptedButtons: Qt.LeftButton

                function getMouseGlobalX(mouse) {
                    const point = mapToGlobal(mouse.x, mouse.y);
                    return point.x;
                }
                
                onPressed: mouse => {
                    pressX = getMouseGlobalX(mouse);
                    console.log(`Pressed ${pressX}`);
                    pressWidth = parent.rectWidth;
                }
                
                onReleased: mouse => {
                    const x = getMouseGlobalX(mouse);
                    console.log(`Released ${x}`);
                    parent.rectWidth = pressWidth + x - pressX;
                    pressX = 0;
                }
                
                onMouseXChanged: mouse => {
                    const x = getMouseGlobalX(mouse);
                    console.log(`Moved ${x}`);
                    if (pressed) {
                        parent.rectWidth = pressWidth + x - pressX;
                        // console.log("Actual: ", parent.width, ", Expected: ", pressWidth + mouse.x - pressX)
                    }
                }

                Rectangle {
                    anchors.fill: parent;
                    color: "red";
                }
            }
        }
 
    //    Rectangle {
    //         color: "transparent"
    //         Layout.fillHeight: true
    //         Layout.fillWidth: true
    //         Layout.row: 1
    //         Layout.column: 1

    //         MouseArea {
    //             anchors.fill: parent

    //             acceptedButtons: Qt.LeftButton | Qt.RightButton
                
    //             onClicked: {
    //                 if (mouse.button === Qt.RightButton){
    //                     console.log("hello")
    //                     contextMenu.popup()
    //                 }
    //             }

    //             onPressAndHold: {
    //                 if (mouse.source === Qt.MouseEventNotSynthesized)
    //                     contextMenu.popup()
    //             }

    //             Menu {
    //                 id: contextMenu
    //                 MenuItem { text: "Cut" }
    //                 MenuItem { text: "Copy" }
    //                 MenuItem { text: "Paste" }
    //             }
    //         }
    //    }
       
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


    //    Rectangle {
    //         color: "blue"
    //         Layout.fillWidth: true
    //         Layout.columnSpan: 3
    //         Layout.rowSpan: 1
    //         Layout.row: 2
    //         Layout.column: 0
    //    }
 
    //    Rectangle {
    //         color: "green"
    //         Layout.fillHeight: true
    //         Layout.fillWidth: true
    //         Layout.columnSpan: 1
    //         Layout.rowSpan: 2
    //         Layout.row: 2
    //         Layout.column: 3
    //    }
 
    //    Rectangle {
    //         color: "white"
    //         Layout.fillHeight: true
    //         Layout.fillWidth: true
    //         Layout.columnSpan: 1
    //         Layout.rowSpan: 1
    //         Layout.row: 2
    //         Layout.column: 2
    //    }
 
    //    Rectangle {
    //         color: "yellow"
    //         Layout.fillHeight: true
    //         Layout.fillWidth: true
    //         Layout.columnSpan: 2
    //         Layout.rowSpan: 1
    //         Layout.row: 3
    //         Layout.column: 1
    //    }
   }
}
