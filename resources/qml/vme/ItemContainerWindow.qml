import QtQuick.Controls 2.15
import QtQuick 2.15
import "./item_container_window" as Components
import "./" as Vme
import Vme.context 1.0 as Context

Item {
  id: itemContainer;

  signal updateLayout();

  required property var model;
  required property int index;

  readonly property int headerHeight : 16;

  readonly property int fixedWidth : 36 * 4;
  readonly property int minHeight : headerHeight + 36;

  property int capacity : {
    return model ? model.capacity : 0;
  }

  property double maxHeight: itemContainerView.cellHeight * Math.ceil(capacity / 4) + header.height;
  property double visibleHeight: itemContainerView.cellHeight * Math.ceil(capacity / 4) + header.height;

  property int currentHeight: Math.round(gridWrapper.visible ? Math.min(visibleHeight,maxHeight) : header.height);

  property bool showContainerItems: true;

  implicitWidth: fixedWidth;
  implicitHeight: currentHeight;

  onImplicitHeightChanged: {
    updateLayout();
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
      text: model.containerName
      color: "#909090"
    }

    Rectangle {
      width: 15
      height: 15
      border.color: "#3b3d40"
      anchors.right: parent.right
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
          itemContainer.showContainerItems = !itemContainer.showContainerItems
        }
      }
    }
  }

  Component {
    id: itemDelegate

    Rectangle {
      id: itemSlot

      required property int serverId
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
            return itemSlot.serverId != -1 ? "image://itemTypes/" + itemSlot.serverId : ""
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
            itemContainer.model.containerItemClicked(itemSlot.index);
            itemContainer.updateLayout();
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

    visible: itemContainer.showContainerItems

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
      focus: true

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

        onPressed: {
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
