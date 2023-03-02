import QtQuick 2.12
import QtQuick.Controls
import AppComponents

Rectangle {
    id: root

    border.color: "#90A4AE"

    property int minWidth: 70
    property int minHeight: 70
    readonly property var extraWidth: 0
    property var rectWidth: implicitWidth
    property var rectHeight: implicitHeight

    // clip: false;

    // Right
    MouseArea {
        property var pressWidth
        property var pressX

        width: 5
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.rightMargin: -2
        hoverEnabled: true
        cursorShape: containsMouse ? Qt.SizeHorCursor : Qt.ArrowCursor
        acceptedButtons: Qt.LeftButton

        // Rectangle {
        //     anchors.fill: parent;
        //     color: "blue";
        // }
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
            parent.rectWidth = extraWidth + Math.round(Math.max(parent.minWidth, pressWidth + x - pressX) / 32) * 32;
            console.log(parent.rectWidth);
            parent.implicitWidth = rectWidth;
            pressX = 0;
        }

        onMouseXChanged: mouse => {
            const x = getMouseGlobalX(mouse);
            if (pressed) {
                parent.rectWidth = extraWidth + Math.round(Math.max(parent.minWidth, pressWidth + x - pressX) / 32) * 32;
                parent.implicitWidth = rectWidth;
            }
        }
    }

    // Bottom
    MouseArea {
        property var pressHeight
        property var pressY

        height: 5
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.bottomMargin: -2
        hoverEnabled: true
        cursorShape: containsMouse ? Qt.SizeVerCursor : Qt.ArrowCursor
        acceptedButtons: Qt.LeftButton

        // Rectangle {
        //     anchors.fill: parent;
        //     color: "blue";
        // }
        function getMouseGlobalY(mouse) {
            const point = mapToGlobal(mouse.x, mouse.y);
            return point.y;
        }

        onPressed: mouse => {
            pressY = getMouseGlobalY(mouse);
            pressHeight = parent.rectHeight;
        }

        onReleased: mouse => {
            const y = getMouseGlobalY(mouse);
            parent.rectHeight = Math.round(Math.max(parent.minHeight, pressHeight + y - pressY) / 32) * 32;
            console.log(parent.rectHeight);
            parent.implicitHeight = rectHeight;
            pressY = 0;
        }

        onMouseYChanged: mouse => {
            const y = getMouseGlobalY(mouse);
            if (pressed) {
                parent.rectHeight = Math.round(Math.max(parent.minHeight, pressHeight + y - pressY) / 32) * 32;
                parent.implicitHeight = rectHeight;
            }
        }
    }
}
