// import QtQuick.Controls 2.15
import QtQuick.Controls 2.0
import QtQuick 2.15
import QtQuick.Layouts 1.15
import "./vme" as Vme
import Vme.context 1.0 as Context

//   ColumnLayout{
//     spacing: 2

//     Rectangle {
//         Layout.alignment: Qt.AlignCenter
//         color: "red"
//         Layout.preferredWidth: 40
//         Layout.preferredHeight: 40
//     }

//     Rectangle {
//         Layout.alignment: Qt.AlignRight
//         color: "green"
//         Layout.preferredWidth: 40
//         Layout.preferredHeight: 70
//     }

//     Rectangle {
//         Layout.alignment: Qt.AlignBottom
//         Layout.fillHeight: true
//         color: "blue"
//         Layout.preferredWidth: 70
//         Layout.preferredHeight: 40
//     }
// }

ScrollView {
  anchors.fill: parent
  id : propertyContainer
  clip : true
  contentHeight : contents.height

  property var containers

    Rectangle {
          color : "transparent"
          anchors.fill : parent
          border.color : "red"
        }


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
            Context.C_PropertyWindow.countChanged(value);
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

      TableView {
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


        // contentHeight : childrenRect.height

        clip : true
        // boundsBehavior : Flickable.OvershootBounds
        // flow : GridView.LeftToRight
        focus : true


        // Rectangle {
        //   color : "transparent"
        //   anchors.fill : parent
        //   border.color : "red"
        // }

        delegate : Component {
          Vme.ItemContainerWindow {
            id : itemContainerView

            required property var itemModel
            model : itemModel
            
            onUpdateLayout : {
              containersView.forceLayout();
            }
          }
        }
      }
    // }

  }
}