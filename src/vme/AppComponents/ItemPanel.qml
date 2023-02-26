import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import AppComponents as VMEComponent

VMEComponent.ResizableItem {
    id: root;
    
    z: mouseArea.drag.active || mouseArea.pressed ? 2 : 1;

    default property alias _contentChildren: contentLayout.data

    property var prevParent;
    property var onDetach: null;

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
        drag.target: parent

        onDragActiveChanged: () => {
            if(drag.active) {
                console.log("Drag.onDragStarted");
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
        onPressed: {
            // root.beginDrag = Qt.point(root.x, root.y);
        }
        onReleased: {
            const result = parent.Drag.drop();
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
        id: contentLayout;
        anchors.fill: parent;

        // Header
        Rectangle {
            Layout.fillWidth: true;
            height: 28;
            color: "#ECEFF1";


            RowLayout {
                anchors.fill: parent;

                Item {
                    Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter;

                    width: 14;
                    height: 14;

                    Image {
                        anchors.fill: parent;
                        width: 14;
                        height: 14;
                        source: "cross.svg";
                    }
                }
                
                Text {
                    Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter;
                    text: "Item panel";
                }
            }
        }
    }

    // ParallelAnimation {
    //     id: backAnim
    //     SpringAnimation { id: backAnimX; target: root; property: "x"; duration: 500; spring: 2; damping: 0.2 }
    //     SpringAnimation { id: backAnimY; target: root; property: "y"; duration: 500; spring: 2; damping: 0.2 }
    // }
}