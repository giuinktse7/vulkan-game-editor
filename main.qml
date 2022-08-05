import QtQuick
import QtQuick.Layouts
import VulkanGameEditor

Item {
    width: 800;
    height: 600;

    MouseArea {
        anchors.fill: parent
        onClicked: { console.log("hello") }
    }

    // MapViewItem {
    //     anchors.fill: parent
    //     focus: true
    
    //     Keys.onPressed: (event) => { console.log("pressed " + event.key); }
    // }

    CustomTextureItem {
        id: renderer
        anchors.fill: parent
        anchors.margins: 10
    }

    Rectangle {
        color: "transparent"
        anchors.fill: parent

        ColumnLayout {
            spacing: 0

            Rectangle {
                Layout.fillWidth: true
                color: "red"
                Layout.preferredWidth: 40
                Layout.preferredHeight: 40
            }

            Rectangle {
                Layout.fillWidth: true
                color: "green"
                Layout.preferredWidth: 40
                Layout.preferredHeight: 70
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "blue"
                Layout.preferredWidth: 70
                Layout.preferredHeight: 40
            }
        }
    }

//    GridLayout {
//        id: grid
//        anchors.fill: parent
 
//        rows: 3
//        columns: 3
 
//        Rectangle {
//             color: "red"
//             Layout.fillHeight: true
//             Layout.fillWidth: true
//             Layout.columnSpan: 2
//             Layout.rowSpan: 1
//             Layout.row: 1
//             Layout.column: 2
//        }
 
//        Rectangle {
//             color: "blue"
//             Layout.fillHeight: true
//             Layout.fillWidth: true
//             Layout.columnSpan: 1
//             Layout.rowSpan: 2
//             Layout.row: 1
//             Layout.column: 1
//        }
 
//        Rectangle {
//             color: "green"
//             Layout.fillHeight: true
//             Layout.fillWidth: true
//             Layout.columnSpan: 1
//             Layout.rowSpan: 2
//             Layout.row: 2
//             Layout.column: 3
//        }
 
//        Rectangle {
//             color: "white"
//             Layout.fillHeight: true
//             Layout.fillWidth: true
//             Layout.columnSpan: 1
//             Layout.rowSpan: 1
//             Layout.row: 2
//             Layout.column: 2
//        }
 
//        Rectangle {
//             color: "yellow"
//             Layout.fillHeight: true
//             Layout.fillWidth: true
//             Layout.columnSpan: 2
//             Layout.rowSpan: 1
//             Layout.row: 3
//             Layout.column: 1
//        }
//    }
}
