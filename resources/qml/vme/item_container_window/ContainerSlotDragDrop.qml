import QtQuick 2.15
import QtQml 2.15

Item {
  id : root
  width : 36
  height : 36

  signal dragStart();
  signal rightClick();
  signal itemDroppedFromMap(var mapItemBuffer, var dropCallback);

    DropArea {
      anchors.fill : parent
      id : itemDropArea

      onEntered : function (drag) {
        drag.accept();
      }

      onDropped : function (drop) {
        var mapItemBuffer = drop.getDataAsArrayBuffer("vulkan-game-editor-mimetype:map-item");
        root.itemDroppedFromMap(mapItemBuffer, (accept) => {
          if (accept) {
            drop.accept();
          }
        });
      }

      Rectangle {
        width: 32
        height: 32

        color : "transparent"
        border.color : "#FFB74D"
        border.width : 1

        visible : parent.containsDrag
      }
    }

    MouseArea {
      acceptedButtons : Qt.LeftButton | Qt.RightButton
      anchors.fill : parent
      propagateComposedEvents : true
      property point pressOrigin
      property int dragStartThreshold : 7
      property bool dragging : false


      onPressed : (mouse) => {
        if (mouse.button & Qt.LeftButton) {
          pressOrigin = Qt.point(mouseX, mouseY);
          dragging = false;
          console.log("Press");
        } else if (mouse.button & Qt.RightButton) {
          rightClick();
        }
      }

      onReleased : {
        pressOrigin = Qt.point(0, 0);
        dragging = false;
      }

      onPositionChanged : {
        if (
          (pressedButtons & Qt.LeftButton) && !dragging
        ) {
          const dx = pressOrigin.x - mouseX
          const dy = pressOrigin.y - mouseY
          if (Math.sqrt(dx * dx + dy * dy) >= dragStartThreshold) {
            root.dragStart();
            dragging = true;
          }
        }
      }
    }
  }
