import QtQuick 2.12
import QtQuick.Controls
import AppComponents
import VME.dataModel 1.0

Rectangle {
    id: root

    default property alias _contentChildren: contentArea.data
    color: "red"
    border.color: "#FF5722"
    border.width: 2

    // border.color: "#90A4AE"
    // border.color: "#616161"
    property int minWidth: 70
    property int minHeight: 70
    property var rectWidth: implicitWidth
    property var rectHeight: implicitHeight

    readonly property var calculateSize: (minValue, startSize, startValue, currentValue) => {
        // + 4 for the border and + 17 for the scrollbar
        const size = Math.round(Math.max(minValue, startSize + currentValue - startValue) / 32) * 32 + 4 + 17;
        return size;
    }

    // clip: false;
    Item {
        id: contentArea
        anchors.fill: parent
        anchors.margins: 2
    }

    // Right
    MouseArea {
        property var pressWidth
        property var pressX

        readonly property var calculateWidth: mouse => {
            const x = getMouseGlobalX(mouse);
            // + 4 for the border and + 17 for the scrollbar
            const result = Math.round(Math.max(parent.minWidth, pressWidth + x - pressX) / 32) * 32 + 4 + 17;
            return result;
        }

        width: 2
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.right: parent.right
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
            // - 4 for the border and - 17 for the scrollbar
            pressWidth = parent.rectWidth - 4 - 17;
            AppDataModel.setCursorShape(Qt.SizeHorCursor);
        }

        onReleased: mouse => {
            parent.rectWidth = calculateWidth(mouse);
            parent.implicitWidth = rectWidth;
            pressX = 0;
            AppDataModel.resetCursorShape();
        }

        onMouseXChanged: mouse => {
            if (pressed) {
                parent.rectWidth = calculateWidth(mouse);
                parent.implicitWidth = rectWidth;
            }
        }
    }

    // // Bottom
    MouseArea {
        property var pressHeight
        property var pressY

        height: 2
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        hoverEnabled: true
        cursorShape: containsMouse ? Qt.SizeVerCursor : Qt.ArrowCursor
        acceptedButtons: Qt.LeftButton

        readonly property var calculateHeight: mouse => {
            const y = getMouseGlobalY(mouse);
            const result = Math.round(Math.max(parent.minHeight, pressHeight + y - pressY) / 32) * 32;
            return result;
        }

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
            AppDataModel.setCursorShape(Qt.SizeVerCursor);
        }

        onReleased: mouse => {
            const y = getMouseGlobalY(mouse);
            parent.rectHeight = calculateHeight(mouse);
            parent.implicitHeight = rectHeight;
            pressY = 0;
            AppDataModel.resetCursorShape();
        }

        onMouseYChanged: mouse => {
            const y = getMouseGlobalY(mouse);
            if (pressed) {
                parent.rectHeight = calculateHeight(mouse);
                parent.implicitHeight = rectHeight;
            }
        }
    }
}
