import QtQuick 2.12
import QtQuick.Controls
import AppComponents
import VME.dataModel 1.0

// TableView {
//     id: tableView
//     anchors.fill: parent
//     columnSpacing: 1
//     rowSpacing: 1
//     clip: true

//     delegate: Rectangle {
//         implicitWidth: 32
//         implicitHeight: 32
//         Image { source: imageUri; }
//     }
// }
Item {
    property alias model: tableView.model

    Component {
        id: brushDelegate

        Image {
            id: item

            width: 32
            height: 32

            source: imageUri

            MouseArea {
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                anchors.fill: parent
                propagateComposedEvents: true

                onPressed: () => {
                    AppDataModel.selectBrush(tableView.model, index);
                }
            }
        }
    }

    GridView {
        id: tableView

        delegate: brushDelegate
        clip: true

        anchors.fill: parent
        cellWidth: 32
        cellHeight: 32
        focus: true
        flickDeceleration: 10000
        snapMode: GridView.SnapToRow

        boundsMovement: Flickable.StopAtBounds
        boundsBehavior: Flickable.StopAtBounds

        ScrollBar.vertical: ScrollBar {
            minimumSize: 0.05
            snapMode: ScrollBar.SnapAlways
            stepSize: 0.01
            // background: Item { opacity: 0; }
            // contentItem: Rectangle { implicitWidth: 10; implicitHeight: 10; color: "blue" }

            // contentItem: Rectangle {
            //     implicitWidth: 12
            //     color: "#757575"
            // }

            // background: Item {}
        }
    }
}
