import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

TabButton {
    id: closeButtonTabButton
    text: qsTr("Tab Title")

    property bool active

    signal close

    contentItem: RowLayout {
        anchors.fill: parent
        spacing: 3

        Text {
            Layout.fillWidth: true

            text: closeButtonTabButton.text
            font: closeButtonTabButton.font
            color: closeButtonTabButton.active ? "#333333" : "#6a6a6a"
            opacity: closeButtonTabButton.enabled ? 1.0 : 0.5
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignLeft
            elide: Text.ElideRight
            padding: 5
        }

        Rectangle {
            id: crossContainer
            color: "transparent"

            width: 16
            height: 16

            radius: 6

            MouseArea {
                id: closePanelMouseArea
                anchors.fill: parent
                hoverEnabled: true

                acceptedButtons: Qt.LeftButton

                onClicked: mouse => {
                    if (mouse.button === Qt.LeftButton) {
                        closeButtonTabButton.close();
                    }
                }
            }

            Image {
                anchors.centerIn: parent
                width: 10
                height: 10
                sourceSize: Qt.size(width, height)
                source: "cross.svg"

                states: State {
                    name: "hovered"
                    when: closePanelMouseArea.containsMouse
                    PropertyChanges {
                        target: crossContainer
                        color: "#e9e9e9"
                    }
                }
            }
        }
    }

    background: Rectangle {
        color: closeButtonTabButton.active ? "#FFFFFF" : "#ECECEC"
    }
}
