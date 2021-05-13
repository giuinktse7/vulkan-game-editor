import QtQuick.Controls 2.0
import QtQuick 2.15
import QtQuick.Layouts 1.1
import "./vme" as Vme
import Vme.context 1.0 as Context

ScrollView {
  anchors.fill: parent
  id: propertyContainer

  visible: false
  clip : true
  contentHeight : contents.height
  padding: 14

  property var containers

    // Rectangle {
    //       color : "transparent"
    //       anchors.fill : parent
    //       border.color : "red"
    //     }


  // ColumnLayout {
    // anchors.left: parent.left
    // anchors.right: parent.right
    ColumnLayout {
      id : contents
      Layout.alignment : Qt.AlignTop
      Layout.fillHeight : false
      Layout.margins : 16

      Layout.preferredWidth : parent.width

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

          onValueChanged: {
            if (visible) {
              Context.C_PropertyWindow.setFocusedItemCount(value);
            }
          }

          onEditingFinished: {
            Context.C_PropertyWindow.setFocusedItemCount(value, true);
          }
        }
      }

      // Text {
      //   font {
      //     pointSize : 7
      //     family : Vme.Constants.labelFontFamily
      //     capitalization : Font.AllUppercase
      //   }
      //   text : "Text 1"
      //   color : "#b7bcc1"
      // }
      // Text {
      //   font {
      //     pointSize : 7
      //     family : Vme.Constants.labelFontFamily
      //     capitalization : Font.AllUppercase
      //   }
      //   text : "Text 2"
      //   color : "#b7bcc1"
      // }
      // Text {
      //   font {
      //     pointSize : 7
      //     family : Vme.Constants.labelFontFamily
      //     capitalization : Font.AllUppercase
      //   }
      //   text : "Text 3"
      //   color : "#b7bcc1"
      // }

      ListView {
        id : containersView
        objectName : "item_container_area"
        model : propertyContainer.containers;
        readonly property int fixedWidth : 36 * 4

        visible : model.size > 0

        Layout.alignment : Qt.AlignTop

        Layout.fillWidth : true
        Layout.minimumWidth : fixedWidth
        Layout.maximumWidth : fixedWidth

        width : 100
        implicitHeight : contentItem.height;

        // reuseItems : true

        clip : true
        focus : true

        delegate : Component {
          Vme.ItemContainerWindow {
            id : itemContainerView

            required property var itemModel

            model : itemModel
            
            onClose: () => {
              containersView.model.closeContainer(index);
            }
          }
        }
      }

  }
}