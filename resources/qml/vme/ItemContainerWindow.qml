import QtQuick.Controls 2.15
import QtQuick.Layouts 1.1
import QtQuick 2.15
import "./item_container_window" as Components
import "./" as Vme
import Vme.context 1.0 as Context

Item {
  id: itemContainer;

  signal updateLayout();
  signal close();

  required property var model;
  required property int index;

  readonly property int headerHeight : 16;

  readonly property int fixedWidth : 36 * 4;
  readonly property int minHeight : headerHeight + 36;

  property int capacity : {
    return model ? model.capacity : 0;
  }

  property double maxHeight: itemContainerView.cellHeight * Math.ceil(capacity / 4) + headerHeight;
  property double visibleHeight: itemContainerView.cellHeight * Math.ceil(capacity / 4) + headerHeight;

  property int currentHeight: {
    return Math.round(!minimized ? Math.min(visibleHeight, maxHeight) : headerHeight)
  };

  property bool minimized: false;

  implicitWidth: fixedWidth;
  // implicitHeight: header.height;

  height: currentHeight;

  onHeightChanged: {
    // updateLayout();
  }

  // Header
  Rectangle {
    id: header
    anchors.left: parent.left
    anchors.right: parent.right
    anchors.top: parent.top
    height: itemContainer.headerHeight
    color: "#3d3e40"

    Image {
      anchors.verticalCenter: parent.verticalCenter
      width: 13
      height: 13
      source: {
        if (model === null) return "";

        const serverId = model.containerServerId;
        return serverId != -1 ? "image://itemTypes/" + serverId : "";
      }
    }

    Text {
      x: parent.x + 15
      anchors.verticalCenter: parent.verticalCenter
      font {
        pointSize: 8
        weight: Font.Bold
        // family: Vme.Constants.labelFontFamily
        family: "Verdana"
         capitalization : Font.Capitalize
      }
      renderType: Text.NativeRendering
      antialiasing: false
      text: {
        return model === null ? "No model :(" : model.containerName;
      }
      
      color: "#909090"
    }

    RowLayout {
      anchors.right: parent.right
      spacing: 1
      
      Rectangle {
        width: 15
        height: 15
        border.color: "#3b3d40"
        color: "#18191a"
        Text {
          anchors.centerIn: parent
          font {
            pointSize: 22
            family: Vme.Constants.labelFontFamily
            capitalization: Font.AllUppercase
          }

          text: "-"
          color: "#ccc"
        }

        MouseArea {
          anchors.fill: parent

          onPressed: {
            // itemContainerView.parent.height = 0;
            itemContainer.minimized = !itemContainer.minimized
          }
        }
      }

      Rectangle {
        visible: itemContainer.index != 0
        width: 15
        height: 15
        border.color: "#3b3d40"
        color: "#18191a"
        Text {
          anchors.centerIn: parent
          font {
            pointSize: 12
            family: Vme.Constants.labelFontFamily
          }

          text: "x"
          color: "#ccc"
        }

        MouseArea {
          anchors.fill: parent

          onPressed: {
            itemContainer.close();
          }
        }
      }
    }

    
  }

  Component {
    id: itemDelegate

    Rectangle {
      id: itemSlot

      required property int serverId
      required property int subtype
      required property int index

      // width : itemContainerView.cellWidth
      // height : itemContainerView.cellHeight
      width: 36
      height: 36

      color: "transparent"

      Rectangle {
        width: itemContainerView.cellWidth - 4
        height: itemContainerView.cellHeight - 4

        anchors.centerIn: parent
        color: "#333"

        Image {
          anchors.fill: parent
          source: {
            // console.log(itemSlot.serverId);
            // return itemSlot.serverId != -1 ? "image://itemTypes/" + itemSlot.serverId + ":" + itemSlot.subtype : ""
            return  Context.C_PropertyWindow.getItemPixmapString(itemSlot.serverId, itemSlot.subtype);
          }
        }

        Components.ContainerSlotDragDrop {
          onItemDroppedFromMap: function (mapItemBuffer, dropCallback) {
            const accepted = itemContainer.model.itemDropEvent(itemSlot.index, mapItemBuffer);
            dropCallback(accepted);
          }

          onDragStart: {
            if (itemSlot.index < itemContainer.model.size) {
              itemContainer.model.itemDragStartEvent(itemSlot.index);
            }
          }

          onRightClick: {
            itemContainer.model.containerItemRightClicked(itemSlot.index);
            // itemContainer.updateLayout();
          }

          onLeftClick: {
            itemContainer.model.containerItemLeftClicked(itemSlot.index);
            // itemContainer.updateLayout();
          }
        }

      }
    }
  } // Component: itemDelegate

  Rectangle {
    id: gridWrapper
    color: "#777"

    anchors.top: header.bottom
    anchors.bottom: parent.bottom

    width: parent.width

    visible: !itemContainer.minimized

    GridView {
      id: itemContainerView
      model: itemContainer.model

      property int maxRows: itemContainer.capacity

      anchors.fill: parent

      cellWidth: 36
      cellHeight: 36
      // contentHeight : 36 * Math.ceil(model.rowCount() / 4)
      contentHeight: 36 * 2
      clip: true
      // boundsBehavior : Flickable.StopAtBounds
      boundsBehavior: Flickable.OvershootBounds
      flow: GridView.LeftToRight
      snapMode: ListView.NoSnap

      delegate: itemDelegate

      ScrollBar.vertical: ScrollBar {
        visible: itemContainer.currentHeight < itemContainer.maxHeight
        policy: ScrollBar.AlwaysOn
        parent: itemContainerView.parent
        anchors.top: itemContainerView.top
        anchors.left: itemContainerView.right
        anchors.bottom: itemContainerView.bottom

        contentItem: Rectangle {
          implicitWidth: 10
          implicitHeight: 64
          radius: 0
          color: parent.pressed ? "#606060" : "#cdcdcd"

          Behavior on color {
            ColorAnimation {
              duration: 250
            }
          }
        }
      }

      // ScrollBar

      // Vertical resize
      // MouseArea {
      // id : bottomMouseArea
      // property int oldMouseY
      // property int startHeight
      // cursorShape : containsMouse || pressed ? Qt.SizeVerCursor : Qt.ArrowCursor
      // preventStealing : true

      // y : parent.y + parent.height - 5 / 2;

      // Rectangle {
      //     anchors.fill : parent
      //     color : "red";
      // }

      // width : itemContainer.width
      // height : 5
      // hoverEnabled : true

      // onPressed : {
      //     applicationContext.setCursor(Qt.SizeVerCursor);
      //     oldMouseY = mouseY;
      //     startHeight = itemContainer.visibleHeight;
      // }
      // onReleased : {
      //     applicationContext.resetCursor();
      // }

      // onPositionChanged : {
      //     if (pressed) {
      //       const deltaY = (mouseY - oldMouseY);
      //       const newHeight = Math.max(itemContainer.minHeight, Math.min(startHeight + deltaY, itemContainer.maxHeight));
      //       itemContainer.visibleHeight = newHeight;
      //     }
      // }
      // }
      MouseArea {
        id: bottomMouseArea
        property int oldMouseY
        property int startHeight
        property bool resizing : false;
        cursorShape: containsMouse
                     || pressed ? Qt.SizeVerCursor : Qt.ArrowCursor
        preventStealing: true
        propagateComposedEvents: true

        // Rectangle {
        //   anchors.fill: parent
        //   color: "red"
        // }

        anchors.fill: parent
        hoverEnabled: true

        onPressed: (mouse) => {
          if ((mouseY > height - 5)) {
            oldMouseY = mouseY
            startHeight = itemContainer.visibleHeight
            resizing = true;
            applicationContext.setCursor(Qt.SizeVerCursor)
          } else {
            mouse.accepted = false;
          }
        }
        
        onReleased: {
          if (resizing) {
            applicationContext.resetCursor()
            resizing = false;
          }
        }

        onPositionChanged: {
          cursorShape = ((mouseY > height - 5) || resizing) ? Qt.SizeVerCursor : Qt.ArrowCursor;

          if (resizing) {
            const deltaY = (mouseY - oldMouseY)
            const newHeight = Math.max(itemContainer.minHeight,
                                       Math.min(startHeight + deltaY,
                                                itemContainer.maxHeight))
            itemContainer.visibleHeight = newHeight
          }
        }
      }
    } // GridView
  }

} // Rectangle
