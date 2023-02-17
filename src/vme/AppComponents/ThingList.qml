import QtQuick 2.12
import AppComponents

TableView {
    anchors.fill: parent
    columnSpacing: 1
    rowSpacing: 1
    clip: true

    delegate: Rectangle {
        implicitWidth: 32
        implicitHeight: 32
        Image { source: imageUri; }
    }
}