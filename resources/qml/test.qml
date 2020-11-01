import QtQuick.Controls 2.15
import QtQuick 2.15
import QtQuick.Layouts 1.15

ColumnLayout {
  anchors.fill : parent
  spacing : 2

  Rectangle {
    TextField {
      anchors.top : parent.top
      placeholderText : qsTr("Count")
    }
  }

}
