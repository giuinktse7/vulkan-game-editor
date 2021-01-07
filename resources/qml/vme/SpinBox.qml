import QtQuick.Controls 2.15
import QtQuick 2.15
import QtQuick.Layouts 1.15
import "./spinbox" as Components

Control {
  id : vmeSpinBox
  property int value : 1
  property int from : 1
  property int to : 100
  property bool editable : true

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

  // Drop shadow for the input field
  // DropShadow {
  //   anchors.fill : childContainer
  //   visible : input.focus
  //   transparentBorder : true
  //   horizontalOffset : 0
  //   verticalOffset : 0
  //   spread : 0.5
  //   source : childContainer
  //   radius : 8
  //   samples : 17
  //   // color : "#2196F37F"
  //   // color : "#2196F348"
  //   color : "#1F2196F3"
  // }

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

        validator: RegularExpressionValidator { regularExpression: /(?:^[1-9][0-9]?[0-9]?$)?/ }

        onTextChanged : {
          if (text !== "") {
            if (parseInt(text) != vmeSpinBox.value) {
              vmeSpinBox.setValue(parseInt(text));
            }
          }
        }

        onEditingFinished : {
          if (text === "") {
            vmeSpinBox.setValue(1);
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
