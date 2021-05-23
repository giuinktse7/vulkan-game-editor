import QtQuick.Controls 2.15
import QtQuick 2.15
import QtQuick.Layouts 1.15
import "./spinbox" as Components

Control {
  id : vmeSpinBox
  property int value: 1
  property int from: 1
  property int to: 100
  property bool editable: true
  property var validator: null

  Layout.minimumWidth : 100
  Layout.preferredWidth : 100
  Layout.maximumWidth : 150
  Layout.minimumHeight : 23
  Layout.preferredHeight : 23

  signal editingFinished();

  function setValue(value) {
    vmeSpinBox.value = Math.max(Math.min(value, vmeSpinBox.to), vmeSpinBox.from);
  }

  function valueAsText() {
    return textFromValue(vmeSpinBox.value, vmeSpinBox.locale);
  }

  function textFromValue(value, locale) {
    return Number(value).toLocaleString(locale, 'f', 0);
  }


  onValueChanged : {
    input.text = valueAsText();
  }

  Rectangle {
    id : childContainer
    anchors.fill : parent

    Rectangle {
      id : inputContainer
      // width : parent.width - 20
      anchors.left : parent.left
      anchors.right : parent.right
      height : parent.height
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
        id : input
        anchors.fill : parent
        selectByMouse : true
        z : 2
        text : vmeSpinBox.valueAsText()

        validator: IntValidator { bottom: vmeSpinBox.from; top: vmeSpinBox.to; }

        onTextChanged : {
          if (text !== "") {
            if (parseInt(text) != vmeSpinBox.value) {
              vmeSpinBox.setValue(parseInt(text));
            }
          }
        }

        onEditingFinished : {
          if (text === "") {
            vmeSpinBox.setValue(vmeSpinBox.from);
            text = vmeSpinBox.valueAsText();
          }

          vmeSpinBox.editingFinished();
        }

        font : vmeSpinBox.font
        color : "#2b2b2b"

        // selectionColor : "#21be2b"
        // selectedTextColor : "#ffffff"
        horizontalAlignment : Qt.AlignHCenter
        verticalAlignment : Qt.AlignVCenter

        readOnly : !vmeSpinBox.editable
        // validator : vmeSpinBox.validator
        inputMethodHints : Qt.ImhFormattedNumbersOnly

      }
    }

    // Components.IndicatorButton {
    // anchors.left : inputContainer.right
    // implicitHeight : parent.height / 2
    // implicitWidth : 20

    // text : "+"
    // onClicked : vmeSpinBox.setValue(vmeSpinBox.value + 1);
    // }
    // Components.IndicatorButton {
    // x : parent.width - 20
    // y : parent.height / 2
    // height : parent.height / 2
    // implicitWidth : 20

    // text : "-"
    // onClicked : vmeSpinBox.setValue(vmeSpinBox.value - 1);
    // }
  }

}
