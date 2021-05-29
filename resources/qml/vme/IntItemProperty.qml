import QtQuick.Controls 2.15
import QtQuick 2.15
import QtQuick.Layouts 1.15
import "./" as Vme


Column {
    id: intItemProperty

    signal editingFinished(int value);

    required property string label;
    required property string inputObjectName;

    required property int from;
    required property int to;
    property int inputWidth: 80;

    property int value: input.value

    spacing: 4

    Text {
        Layout.alignment : Qt.AlignTop

        text : intItemProperty.label
        color : "#565e65"

        font {
            pointSize : 7
            family : Vme.Constants.labelFontFamily
            capitalization : Font.AllUppercase
        }
    }

    Rectangle {
        Layout.alignment : Qt.AlignTop
        Layout.fillHeight: true

        // width: 70
        width: intItemProperty.inputWidth
        height: 25

        border.color : {
            if (input.focus) {
                return "#2196F3";
            }

            if (inputMouseArea.containsMouse) {
                return "#aaa";
            }

            return "#ccc";
        }

        MouseArea {
            id : inputMouseArea
            anchors.fill : parent
            cursorShape : Qt.IBeamCursor

            hoverEnabled : true
        }

        TextInput {
            anchors.fill: parent
            id: input;
            leftPadding: 10

            text: intItemProperty.from;

            focus: true
            activeFocusOnTab: true
            selectByMouse: true
            z: 2
            verticalAlignment : Qt.AlignVCenter

            color: Vme.Constants.labelTextColor

            property int value: {
                if (acceptableInput) {
                    return parseInt(text)
                } else {
                    return value || intItemProperty.from
                }
            };

            validator: RegularExpressionValidator { regularExpression: /(?:^0|[1-9][0-9]*$)?/ }

            onEditingFinished: () => {
                console.log(parseInt(text))
                if (parseInt(text) < intItemProperty.from) {
                    text = intItemProperty.from
                } else if (parseInt(text) > intItemProperty.to) {
                    text = intItemProperty.to
                }

                intItemProperty.editingFinished(value);
            }

            objectName: intItemProperty.inputObjectName
            inputMethodHints : Qt.ImhFormattedNumbersOnly
        }
    }
}