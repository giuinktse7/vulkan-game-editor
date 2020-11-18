import QtQuick.Controls 2.15
import QtQuick 2.15
import QtQuick.Layouts 1.15
import "./vme" as Vme

ScrollView {
  id : propertyContainer
  anchors.fill : parent
  clip : true
  contentHeight : contents.height

  // Items in a ContainerItem
  property var containerItems

  // Rectangle {
  // x : parent.x
  // y : parent.y
  // width : parent.width
  // height : parent.height

  // border.color : "red"
  // }

  ColumnLayout {
    ColumnLayout {
      id : contents
      Layout.alignment : Qt.AlignTop
      Layout.fillHeight : false
      Layout.margins : 16

      Layout.preferredWidth : parent.width
      // Layout.preferredHeight : childrenRect.height

      // Rectangle {
      // x : parent.x
      // y : parent.y
      // width : parent.width
      // height : parent.height

      // border.color : "green"
      // }

      // Rectangle {
      // Layout.alignment : Qt.AlignTop
      // Layout.fillHeight : false

      // property int preferredHeight : 40

      // Layout.preferredHeight : preferredHeight
      // Layout.preferredWidth : 40


      // border.color : "green"

      // MouseArea {
      //     property int oldMouseY
      //     cursorShape : containsMouse || pressed ? Qt.SizeVerCursor : Qt.ArrowCursor
      //     preventStealing : true

      //     anchors.left : parent.left
      //     anchors.right : parent.right
      //     y : parent.y + parent.height - 5 / 2
      //     width : parent.width
      //     height : 5
      //     hoverEnabled : true

      //     onPressed : {
      //       applicationContext.setCursor(Qt.SizeVerCursor);
      //       oldMouseY = mouseY;
      //     }
      //     onReleased : {
      //       applicationContext.resetCursor();
      //     }

      //     onPositionChanged : {
      //       if (pressed) {
      //         const newHeight = Math.max(36, Math.min(parent.height + (mouseY - oldMouseY), 80));
      //         parent.preferredHeight = newHeight;
      //       }
      //     }
      // }
      // }

      Rectangle {
        width : 32
        height : 32
        border.color : "red"
        DropArea {
          id : itemDropArea
          property string imageUrl : ""
          x : 0
          y : 0
          width : 32
          height : 32

          onDropped : {
            console.log("DropArea::onDropped");

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


      ColumnLayout {
        Layout.alignment : Qt.AlignTop
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
        Layout.alignment : Qt.AlignTop
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
        Layout.alignment : Qt.AlignTop
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

      Text {
        font {
          pointSize : 7
          family : Vme.Constants.labelFontFamily
          capitalization : Font.AllUppercase
        }
        text : "Text 1"
        color : "#b7bcc1"
      }
      Text {
        font {
          pointSize : 7
          family : Vme.Constants.labelFontFamily
          capitalization : Font.AllUppercase
        }
        text : "Text 2"
        color : "#b7bcc1"
      }
      Text {
        font {
          pointSize : 7
          family : Vme.Constants.labelFontFamily
          capitalization : Font.AllUppercase
        }
        text : "Text 3"
        color : "#b7bcc1"
      }

      ColumnLayout {
        id : itemContainerArea
        Layout.alignment : Qt.AlignTop
        Layout.minimumWidth : itemContainerView.fixedWidth
        visible : false

        objectName : "item_container_area"
        Layout.bottomMargin : 12
        spacing : 0

        property bool showContainerItems : true

        // Header
        Rectangle {
          Layout.fillWidth : true
          Layout.preferredHeight : 16
          color : "#3d3e40"

          Image {
            anchors.verticalCenter : parent.verticalCenter
            width : 12
            height : 12
            source : {
              const serverId = 1987;
              return serverId != -1 ? "image://itemTypes/" + serverId : "";
            }
          }

          Text {
            x : parent.x + 14
            anchors.verticalCenter : parent.verticalCenter
            font {
              pointSize : 8
              family : Vme.Constants.labelFontFamily
              // capitalization : Font.AllUppercase
            }
            text : "Contents"
            color : "#ccc"
          }

          Rectangle {
            width : 15
            height : 15
            border.color : "#3b3d40"
            anchors.right : parent.right
            color : "#18191a"
            Text {
              anchors.centerIn : parent
              font {
                pointSize : 22
                family : Vme.Constants.labelFontFamily
                capitalization : Font.AllUppercase
              }

              text : "-"
              color : "#ccc"
            }

            MouseArea {
              anchors.fill : parent

              onPressed : {
                itemContainerArea.showContainerItems = !itemContainerArea.showContainerItems;
              }
            }
          }
        } // Header


        Vme.ItemContainerWindow {
          id : itemContainerView
          model : propertyContainer.containerItems

          visible : itemContainerArea.showContainerItems

          Layout.alignment : Qt.AlignTop

          Layout.fillWidth : true
          Layout.maximumWidth : fixedWidth
          Layout.minimumWidth : fixedWidth
          Layout.minimumHeight : 36
          Layout.preferredHeight : preferredHeight
        }
      }
    }

  }
}
