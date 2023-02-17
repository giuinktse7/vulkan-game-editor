import QtQuick 2.12
import AppComponents

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

GridView {
    id: tableView;
    anchors.fill: parent;
    anchors.rightMargin: 14;
    cellWidth: 32;
    cellHeight: 32;
    focus: true

    boundsMovement: Flickable.StopAtBounds
    boundsBehavior: Flickable.StopAtBounds

    // columnSpacing: 1
    // rowSpacing: 1
    // clip: true

    delegate: Rectangle {
        id: item
        implicitWidth: 32;
        implicitHeight: 32;

        Image { source: imageUri; }

        MouseArea {
            acceptedButtons : Qt.LeftButton | Qt.RightButton
            anchors.fill : parent
            propagateComposedEvents : true
            property point pressOrigin
            property int dragStartThreshold : 7
            property bool dragging : false

            onPressed: (mouse) => {
                tableView.model.indexClicked(index);
            }
        }
    }
}