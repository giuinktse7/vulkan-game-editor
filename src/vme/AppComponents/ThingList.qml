import QtQuick 2.12
import AppComponents

// Rectangle {
//             TextInput { text: "ThingList.qml"; }

// }

TableView {
    anchors.fill: parent
    columnSpacing: 1
    rowSpacing: 1
    clip: true

    model: TileSetModel {}

    delegate: Rectangle {
        implicitWidth: 100
        implicitHeight: 50
        Text {
            text: display
        }
    }
}