import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import AppComponents as VMEComponent
import VME.dataModel 1.0

VMEComponent.ItemPanel {
    x: 400
    y: 200

    // + 4 for the border and + 17 for the scrollbar
    implicitWidth: 300
    implicitHeight: 200

    ColumnLayout {
        Layout.fillWidth: true
        Layout.fillHeight: true

        spacing: 0

        Text {
            text: "Hello Towns!"
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter
            Layout.margins: 5
            font.pixelSize: 20
            color: "white"
        }

        Component {
            id: listDelegate
            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                border.color: "purple"
                border.width: 1
                Text {
                    text: name
                }
            }
        }

        ListView {
            id: list
            width: 180
            height: 200

            model: AppDataModel.townListModel
            delegate: listDelegate
        }

        TextInput {
            id: textInput
            font.family: "Arial"
            font.pointSize: 14
            padding: 10
            selectionColor: "#6666FF"
            cursorVisible: true
            focus: true
        }

        Button {
            text: "New"
            onClicked: AppDataModel.createTown()
        }
    }
}
