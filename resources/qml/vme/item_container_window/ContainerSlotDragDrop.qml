import QtQuick 2.15
import QtQml 2.15

Item {
  id : root
  width : 32
  height : 32

  signal itemDroppedFromMap(var mapItemBuffer, var dropCallback);
    signal dragStart();

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
        anchors.fill : parent

        color : "transparent"
        border.color : "#FFB74D"
        border.width : 1

        visible : parent.containsDrag
      }
    }

    MouseArea {
      anchors.fill : parent
      propagateComposedEvents : true
      property point pressOrigin
      property int dragStartThreshold : 7
      property bool dragging : false


      onPressed : {
        console.log("pressed");
        pressOrigin = Qt.point(mouseX, mouseY);
      }

      onReleased : {
        console.log("released");
        pressOrigin = Qt.point(0, 0);
        dragging = false;
      }

      onPositionChanged : {
        if (pressed) {
          const dx = pressOrigin.x - mouseX
          const dy = pressOrigin.y - mouseY
          if (!dragging && Math.sqrt(dx * dx + dy * dy) >= dragStartThreshold) {
            root.dragStart();
            dragging = true;
          }
        }
      }
    }
  }
