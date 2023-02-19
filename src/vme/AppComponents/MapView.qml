import QtQuick 2.12
import QtQuick.Controls
import AppComponents

QmlMapItem {
    id: root

    // anchors.fill: parent
    // anchors.margins: 0
    focus: true
    onActiveFocusChanged: () => {
        root.setFocus(activeFocus);
    }
    
    property var editor;

    MouseArea {
        anchors.fill: parent;
        acceptedButtons: Qt.LeftButton | Qt.RightButton;
        propagateComposedEvents: true;
        hoverEnabled: true;

        onPositionChanged: (mouse) => {
            root.onMousePositionChanged(mouse.x, mouse.y, mouse.button, mouse.buttons, mouse.modifiers);
        }

        onClicked: (mouse) => {
            mouse.accepted = false;

            if (mouse.button === Qt.RightButton) {
                contextMenu.popup();
            } else if (mouse.button === Qt.LeftButton) {
                root.forceActiveFocus();
            }
        }

        onPressed: (mouse) => {
            mouse.accepted = false;

            if (mouse.button === Qt.RightButton) {
                contextMenu.popup();
            } else if (mouse.button === Qt.LeftButton) {
                root.forceActiveFocus();
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