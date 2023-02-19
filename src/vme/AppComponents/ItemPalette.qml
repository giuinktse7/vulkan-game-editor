import QtQuick 2.12
import AppComponents


Rectangle {
    id: root
    readonly property int minWidth: 70;
    property var rectWidth: 150;
    
    required property var model;
    
    ThingList {
        model: {
            root.model;
        }
    }

    MouseArea {
        readonly property int minWidth: parent.minWidth;

        property var pressWidth;
        property var pressX;
        
        id: roiRectArea;
        width: 5;
        anchors.top: parent.top;
        anchors.bottom: parent.bottom;
        anchors.right: parent.right;
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
            parent.rectWidth = Math.max(minWidth, pressWidth + x - pressX);
            pressX = 0;
        }
        
        onMouseXChanged: mouse => {
            const x = getMouseGlobalX(mouse);
            if (pressed) {
                parent.rectWidth = Math.max(minWidth, pressWidth + x - pressX);
            }
        }

        Rectangle {
            anchors.fill: parent;
            color: "red";
        }
    }
}