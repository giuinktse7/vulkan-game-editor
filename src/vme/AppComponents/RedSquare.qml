import AppComponents
import QtQuick

Rectangle {
    anchors.centerIn: parent

    width: 50;
    height: 50;
    color: "red"

    Text {
        text: k.answer
        anchors.centerIn: parent
        color: "white"
        z: 2
    }
   
    MyType {
        id: k
    }
}
