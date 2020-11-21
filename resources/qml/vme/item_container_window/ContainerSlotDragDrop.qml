import QtQuick 2.15
import QtQml 2.15

Item {
  id : root
  width : 32
  height : 32

  signal itemDroppedFromMap(var mapItemBuffer, var dropCallback)

    DropArea {
      id : itemDropArea
      x : 0
      y : 0
      width : 32
      height : 32

      onEntered : function (drag) {
        drag.accept();
        console.log("Accepted");
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
  }
