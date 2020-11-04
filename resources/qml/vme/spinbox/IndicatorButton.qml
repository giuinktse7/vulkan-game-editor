import QtQuick 2.15
import QtQml 2.15

Rectangle {
  id : indicatorButton
  required property string text
  // color : vmeSpinBox.up.pressed ? "#e4e4e4" : "#f6f6f6"
  border.color : enabled ? "#37474F" : "#37474F"
  signal clicked()

  Text {
    text : indicatorButton.text
    font.pixelSize : vmeSpinBox.font.pixelSize
    // color : "#21be2b"
    anchors.fill : parent
    fontSizeMode : Text.Fit
    horizontalAlignment : Text.AlignHCenter
    verticalAlignment : Text.AlignVCenter


  }

  MouseArea {
    id : buttonArea
    anchors.fill : parent
    pressAndHoldInterval : 500

    onClicked : indicatorButton.clicked()
    onPressAndHold : {
      timer.start();
    }
  }

  Timer {
    id : timer
    interval : 50
    repeat : true
    triggeredOnStart : true
    running : false
    onTriggered : {
      if (!buttonArea.containsPress) {
        stop();
        return;
      }

      indicatorButton.clicked();
    }
  }
}
