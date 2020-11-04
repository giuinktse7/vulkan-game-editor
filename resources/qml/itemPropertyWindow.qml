import QtQuick.Controls 2.15
import QtQuick 2.15
import QtQuick.Layouts 1.15
import "./vme" as Vme

ColumnLayout {
  anchors.fill : parent

  ColumnLayout {
    Layout.alignment : Qt.AlignTop
    Layout.margins : 16
    width : parent.width

    ColumnLayout {
      Layout.bottomMargin : 12

      Text {
        font {
          pointSize : 7
          family : Vme.Constants.labelFontFamily
          capitalization : Font.AllUppercase
        }
        text : "Count"
        color : "#b7bcc1"
      }

      Vme.SpinBox {
        Layout.alignment : Qt.AlignTop

        Layout.minimumWidth : 100
        Layout.preferredWidth : 100
        Layout.maximumWidth : 150
        Layout.minimumHeight : 30
        Layout.preferredHeight : 30

        objectName : "count_spinbox"
        onValueChanged : {
          propertyWindow.countChanged(value);
        }
      }
    }

    ColumnLayout {
      Layout.bottomMargin : 12

      Text {
        font {
          pointSize : 7
          family : Vme.Constants.labelFontFamily
          capitalization : Font.AllUppercase
        }
        text : "Action ID"
        color : "#b7bcc1"
      }

      Vme.SpinBox {
        Layout.alignment : Qt.AlignTop

        from : 0
        to : 65535
        value : 0

        Layout.minimumWidth : 100
        Layout.preferredWidth : 100
        Layout.maximumWidth : 150
        Layout.minimumHeight : 30
        Layout.preferredHeight : 30

        objectName : "action_id_spinbox"
        onValueChanged : {}
      }
    }

    ColumnLayout {
      Layout.bottomMargin : 12

      Text {
        font {
          pointSize : 7
          family : Vme.Constants.labelFontFamily
          capitalization : Font.AllUppercase
        }
        text : "Unique ID"
        color : "#b7bcc1"
      }

      Vme.SpinBox {
        Layout.alignment : Qt.AlignTop

        from : 0
        to : 65535
        value : 0

        Layout.minimumWidth : 100
        Layout.preferredWidth : 100
        Layout.maximumWidth : 150
        Layout.minimumHeight : 30
        Layout.preferredHeight : 30

        objectName : "unique_id_spinbox"
        onValueChanged : {}
      }
    }

  }

}
