import QtQuick 2.15
import QtQml 2.15

Item {
  id : root
  width : 32
  height : 32

  signal itemDroppedFromMap(var mapItemBuffer)

  DropArea {
    id : itemDropArea
    property string imageUrl : ""
    x : 0
    y : 0
    width : 32
    height : 32

    onEntered : function (drag) {
      drag.accept();
    }

    onDropped : function (drop) {
      drop.accept();
      var data = drop.getDataAsArrayBuffer("vulkan-game-editor-mimetype:map-item");
      root.itemDroppedFromMap(data);

      const serverId = 1987;
      imageUrl = serverId != -1 ? "image://itemTypes/" + serverId : "";
    }

    Rectangle {
      anchors.fill : parent
      border.color : "green"
      border.width : 2

      visible : parent.containsDrag
    }
    Image {
      visible : itemDropArea.imageUrl != ""
      anchors.fill : parent
      anchors.verticalCenter : parent.verticalCenter
      source : itemDropArea.imageUrl
    }
  }
}
