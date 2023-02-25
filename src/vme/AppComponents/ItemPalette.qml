import QtQuick 2.12
import QtQuick.Controls
import AppComponents


Rectangle {
    id: root
    readonly property int minWidth: 70;
    readonly property var extraWidth: 0;
    property var rectWidth: extraWidth + 32 * 4;
    clip: false;

    required property var model;

    // ColumnLayout {
        ThingList {
            model: {
                root.model;
            }
        }
    // }
    


    MouseArea {
        readonly property int minWidth: parent.minWidth;

        property var pressWidth;
        property var pressX;
        
        width: 5;
        anchors.top: parent.top;
        anchors.bottom: parent.bottom;
        anchors.right: parent.right;
        anchors.rightMargin: -2;
        hoverEnabled: true;
        cursorShape: containsMouse ? Qt.SizeHorCursor : Qt.ArrowCursor;
        acceptedButtons: Qt.LeftButton

        function getMouseGlobalX(mouse) {
            const point = mapToGlobal(mouse.x, mouse.y);
            return point.x;
        }
        
        onPressed: mouse => {
            pressX = getMouseGlobalX(mouse);
            pressWidth = parent.rectWidth;
        }
        
        onReleased: mouse => {
            const x = getMouseGlobalX(mouse);
            parent.rectWidth = extraWidth + Math.round(Math.max(minWidth, pressWidth + x - pressX) / 32) * 32;
            pressX = 0;
        }
        
        onMouseXChanged: mouse => {
            const x = getMouseGlobalX(mouse);
            if (pressed) {
                parent.rectWidth = extraWidth + Math.round(Math.max(minWidth, pressWidth + x - pressX) / 32) * 32;
            }
        }

        // Rectangle {
        //     anchors.fill: parent;
        //     color: "red";
        // }
    }
}