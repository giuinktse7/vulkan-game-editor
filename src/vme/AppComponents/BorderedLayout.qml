import QtQuick
import QtQuick.Layouts

ColumnLayout {
    spacing: 0
    default property alias _contentChildren: container.data

    //top line border
    Rectangle {
        Layout.fillWidth: true
        height: 1
        color: "#5C6BC0"
    }

    //create row layout for each element inside column layout
    RowLayout {
        RowLayout {
            id: container
            Rectangle {
                Layout.fillHeight: true
                width: 1
                color: "#5C6BC0"
            }
        }

        Rectangle {
            Layout.fillHeight: true
            width: 1
            color: "#5C6BC0"
        }
    }

    //bottom line border
    Rectangle {
        Layout.fillWidth: true
        height: 1
        color: "#5C6BC0"
    }
}
