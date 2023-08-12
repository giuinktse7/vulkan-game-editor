import QtQuick
import QtQuick.Layouts

RowLayout {
    id: root
    property string label
    property string text
    property int minimumInputWidth: 80

    Text {
        text: label ? label + ":" : ""
    }

    Rectangle {
        Layout.minimumWidth: minimumInputWidth
        Layout.preferredWidth: childrenRect.width
        Layout.preferredHeight: childrenRect.height

        border {
            color: '#ececec'
            width: 1
        }

        radius: 5

        TextInput {
            id: textInput
            topPadding: 3
            bottomPadding: 3
            leftPadding: 6
            rightPadding: 6
            width: minimumInputWidth
            selectByMouse: true

            text: root.text

            MouseArea {
                anchors.fill: parent
                hoverEnabled: true
                acceptedButtons: Qt.NoButton
                propagateComposedEvents: true

                cursorShape: Qt.IBeamCursor
            }
        }
    }

    Binding {
        target: textInput
        property: "text"
        value: root.text
    }

    Binding {
        target: root
        property: "text"
        value: textInput.text
    }
}
