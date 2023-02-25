import QtQuick 2.12
import QtQuick.Controls
import QtQuick.Layouts
import AppComponents

ColumnLayout {
    id: root;
    required property int theId;

    RowLayout {
        QmlMapItem {
            id: mapItem


            Layout.fillWidth: true;
            Layout.fillHeight: true;

            focus: true
            onActiveFocusChanged: () => {
                mapItem.setFocus(activeFocus);
            }
            
            property var editor;

            MouseArea {
                anchors.fill: parent;
                acceptedButtons: Qt.LeftButton | Qt.RightButton;
                propagateComposedEvents: true;
                hoverEnabled: true;

                onPositionChanged: (mouse) => {
                    mapItem.onMousePositionChanged(mouse.x, mouse.y, mouse.button, mouse.buttons, mouse.modifiers);
                }

                onClicked: (mouse) => {
                    mouse.accepted = false;

                    if (mouse.button === Qt.RightButton) {
                        contextMenu.popup();
                    } else if (mouse.button === Qt.LeftButton) {
                        mapItem.forceActiveFocus();
                    }
                }

                onPressed: (mouse) => {
                    mouse.accepted = false;

                    if (mouse.button === Qt.RightButton) {
                        contextMenu.popup();
                    } else if (mouse.button === Qt.LeftButton) {
                        mapItem.forceActiveFocus();
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

            Component.onCompleted: () => {
                mapItem.entryId = root.theId;
            }
        }

        ScrollBar {
            size: mapItem.verticalScrollSize;
            position: mapItem.verticalScrollPosition;
            policy: ScrollBar.AlwaysOn;
            orientation: Qt.Vertical;
            active: hovered || pressed;

            Layout.fillHeight: true;

            onPositionChanged: {
                if (active) {
                    mapItem.setVerticalScrollPosition(position);
                }
            }
        }
    }

    ScrollBar {
        size: mapItem.horizontalScrollSize;
        position: mapItem.horizontalScrollPosition;
        policy: ScrollBar.AlwaysOn;
        orientation: Qt.Horizontal;
        active: hovered || pressed;

        Layout.fillWidth: true;
        
        onPositionChanged: {
            if (active) {
                mapItem.setHorizontalScrollPosition(position);
            }
        }
    }
}
