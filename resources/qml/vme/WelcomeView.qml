import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Item {
    width: 300;
    height: 400;

    Rectangle {
        anchors.fill: parent;
        anchors.margins: 14;

        border.color: "#2196F3"
        border.width: 1

        ColumnLayout {
            id: page
            width: Math.min(400, parent.width)
            height: parent.height

            RowLayout {
                Button {
                    text: "Open map"
                    onClicked: {
                        console.log("Open map");
                        _viewModel.selectMapFile();
                    }
                }
            }

            ListView {
                id: list;
                Layout.alignment: Qt.AlignTop
                Layout.maximumWidth: parent.width;
                height: childrenRect.height
                spacing: 7;

                model: _recentFilesModel
                delegate: Row {
                    spacing: 14
                    Rectangle {
                        id: mapImage
                        color: "red"
                        width: 40
                        height: 40
                    }

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        width: list.parent.width - mapImage.width - parent.spacing;
                        elide: Text.ElideRight
                        text: path
                    }
                }
            }
        }
    }

}
