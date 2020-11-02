import QtQuick.Controls 2.15
import QtQuick 2.15
import QtQuick.Layouts 1.15

ColumnLayout {
  anchors.fill : parent
  spacing : 2

  Rectangle {

    SpinBox {
      id : countSpinbox
      value : 1
      from : 1
      to : 100
      editable : true

      validator : RegExpValidator {
        regExp : /(?:^[1-9][0-9]?$|^100$)?/
      }

      // textFromValue : function (value, locale) {
      // return Number(Math.min(value, 100)).toLocaleString(locale, 'f', 0);
      // }
      // valueFromText : function (text, locale) {
      // return Math.min(Number.fromLocaleString(locale, text), 100);
      // }


      contentItem : TextInput {
        selectByMouse : true
        z : 2
        text : countSpinbox.textFromValue(countSpinbox.value, countSpinbox.locale)

        onTextChanged : {
          console.log("Changed to" + text);
          if (text !== "") {
            countSpinbox.value = text === "0" ? 1 : Math.max(Math.min(parseInt(text), 100), 1);
            text = countSpinbox.textFromValue(countSpinbox.value, countSpinbox.locale);
          }
        }

        onEditingFinished : {
          if (text === "") {
            countSpinbox.value = 1;
            text = countSpinbox.textFromValue(countSpinbox.value, countSpinbox.locale);
          }
        }

        font : countSpinbox.font
        // color : "#21be2b"
        // selectionColor : "#21be2b"
        // selectedTextColor : "#ffffff"
        horizontalAlignment : Qt.AlignHCenter
        verticalAlignment : Qt.AlignVCenter

        readOnly : !countSpinbox.editable
        validator : countSpinbox.validator
        // inputMethodHints : Qt.ImhFormattedNumbersOnly
      }

      up.indicator : Rectangle {
        x : countSpinbox.mirrored ? 0 : parent.width - width
        height : parent.height
        implicitWidth : 40
        implicitHeight : 40
        // color : countSpinbox.up.pressed ? "#e4e4e4" : "#f6f6f6"
        border.color : enabled ? "#37474F" : "#37474F"

        Text {
          text : "+"
          font.pixelSize : countSpinbox.font.pixelSize * 2
          // color : "#21be2b"
          anchors.fill : parent
          fontSizeMode : Text.Fit
          horizontalAlignment : Text.AlignHCenter
          verticalAlignment : Text.AlignVCenter
        }
      }

      down.indicator : Rectangle {
        x : countSpinbox.mirrored ? parent.width - width : 0
        height : parent.height
        implicitWidth : 40
        implicitHeight : 40
        // color : countSpinbox.down.pressed ? "#e4e4e4" : "#f6f6f6"
        border.color : enabled ? "#37474F" : "#37474F"


        Text {
          text : "-"
          font.pixelSize : countSpinbox.font.pixelSize * 2
          // color : "#21be2b"
          anchors.fill : parent
          fontSizeMode : Text.Fit
          horizontalAlignment : Text.AlignHCenter
          verticalAlignment : Text.AlignVCenter
        }
      }

      background : Rectangle {
        implicitWidth : 140
        border.color : "red"
      }
    }
  }

}
