import QtQuick 2.1

 MouseArea {
    id: root
    property string text;
    property int fontSize;
    property bool selected;
    property int amount;

    width: hidden_text_for_correct_width.width;
    height: hidden_text_for_correct_width.height;

    Rectangle {
        color: "transparent";
        width: childrenRect.width;
        height: childrenRect.height;

        Text {
            id: hidden_text_for_correct_width
            visible: false;
            text: root.text + " (" + root.amount + ")";
            font.pointSize: root.fontSize
            font.bold: false
        }
        
        Text {
            text: root.text + " (" + root.amount + ")";
            font.pointSize: root.fontSize
            font.bold: root.selected
        }
    }
}