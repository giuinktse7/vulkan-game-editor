import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import AppComponents as VMEComponent

VMEComponent.ResizableItem {
    id: root

    z: mouseArea.drag.active || mouseArea.pressed ? 2 : 1

    default property alias _contentChildren: contentLayout.data

    property var prevParent
    property var onDetach: null

    // property point beginDrag
    property bool caught: false

    radius: 5
    Drag.active: mouseArea.drag.active
    Drag.supportedActions: Qt.MoveAction
    Drag.proposedAction: Qt.MoveAction

    // DropArea {

    // }
    MouseArea {
        id: mouseArea

        property bool dragActive: drag.active

        anchors.fill: parent
        drag.target: root

        onDragActiveChanged: () => {
            if (drag.active) {
                // console.log("Drag.onDragStarted");
                // if (root.parent.beforeRemoveItem) {
                //     root.parent.beforeRemoveItem(root);
                // }
                if (root.onDetach) {
                    root.onDetach();
                    root.onDetach = null;
                }
                root.prevParent = root.parent;
                root.parent = contentRoot;
            }
        }

        onPressed: mouse => {
            root.Drag.hotSpot.x = mouse.x - mouseArea.x;
            root.Drag.hotSpot.y = mouse.y - mouseArea.y;
        }

        onReleased: {
            const result = root.Drag.drop();
            // if (result == Qt.MoveAction) {
            // } else {
            //     backAnimX.from = root.x;
            //     backAnimX.to = beginDrag.x;
            //     backAnimY.from = root.y;
            //     backAnimY.to = beginDrag.y;
            //     backAnim.start()
            // }
        }
    }

    ColumnLayout {
        id: contentLayout
        anchors.fill: parent

        spacing: 0

        // Header
        Rectangle {
            Layout.fillWidth: true
            z: 2
            height: 24
            color: "#f3f3f3"

            Rectangle {
                id: crossContainer
                anchors.right: parent.right
                anchors.rightMargin: 5
                anchors.verticalCenter: parent.verticalCenter
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
                            root.destroy();
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

            // Text {
            //     anchors.centerIn: parent
            //     text: "Item panel"
            // }
        }
    }

    // ParallelAnimation {
    //     id: backAnim
    //     SpringAnimation { id: backAnimX; target: root; property: "x"; duration: 500; spring: 2; damping: 0.2 }
    //     SpringAnimation { id: backAnimY; target: root; property: "y"; duration: 500; spring: 2; damping: 0.2 }
    // }
}
