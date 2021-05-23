import QtQuick.Controls
import QtQuick
import QtQuick.Layouts
import "./vme" as Vme
import Vme.context 1.0 as Context

ScrollView {
    anchors.fill: parent
    id: propertyContainer

    visible: false
    clip: true
    contentHeight: contents.height
    padding: 14

    property var containers
    property var fluidTypeModel

    ColumnLayout {
        id: contents
        Layout.alignment: Qt.AlignTop
        Layout.fillHeight: false
        Layout.margins: 16

        Layout.fillWidth: true

        ColumnLayout {
            Layout.alignment: Qt.AlignTop
            Layout.bottomMargin: 8;

            Image {
                objectName : "property_item_image"
                width: 32
                height: 32
                source: ""
            }
        }

        Vme.IntItemProperty {
            Layout.alignment: Qt.AlignTop;
            Layout.bottomMargin: 8;

            label: "Action ID"
            inputObjectName: "item_actionid_input"
            from: 0
            to: 65535

            onValueChanged: {
                if (visible) {
                    Context.C_PropertyWindow.setPropertyItemActionId(value);
                }
            }

            onEditingFinished: value => {
                Context.C_PropertyWindow.setPropertyItemActionId(value, true);
            }
        }

        Vme.IntItemProperty {
            Layout.alignment: Qt.AlignTop;
            Layout.bottomMargin: 8;

            label: "Unique ID"
            inputObjectName: "item_uniqueid_input"
            from: 0
            to: 65535
        }

        Vme.IntItemProperty {
            id: countProperty
            Layout.alignment: Qt.AlignTop;
            Layout.bottomMargin: 8;

            label: "Count"
            inputObjectName: "item_count_input"
            from: 1
            to: 100
            
            onValueChanged: {
                if (visible) {
                    Context.C_PropertyWindow.setPropertyItemCount(value);
                }
            }

            onEditingFinished: value => {
                Context.C_PropertyWindow.setPropertyItemCount(value, true);
            }
        }
    
        ColumnLayout {
            Layout.preferredWidth: parent.width

            Text {
                Layout.alignment : Qt.AlignTop

                text : "Fluid"
                color : "#565e65"

                font {
                    pointSize : 7
                    family : Vme.Constants.labelFontFamily
                    capitalization : Font.AllUppercase
                }
            }

            Vme.Dropdown {
                id: fluidProperty
                // model: ["None", "WaterWaterWaterWaterWaterWaterWaterWaterWaterWater", "Blood", "Beer", "Slime", "Lemonade", "Milk", "Manafluid", "Water2", "Lifefluid", "Oil", "Slime2", "Urine", "CoconutMilk", "Wine", "Mud", "FruitJuice", "Lava", "Rum", "Swamp"];
                model: propertyContainer.fluidTypeModel;

                objectName: "fluid_type_input"

                onActivated: {
                    Context.C_PropertyWindow.setFluidType(currentIndex);
                }

                onHighlighted: {
                    Context.C_PropertyWindow.fluidTypeHighlighted(highlightedIndex);
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
        model: propertyContainer.containers;
        readonly property int fixedWidth : 36 * 4
        onActiveFocusChanged: {console.log("activeFocus: ", activeFocus);}

        MouseArea {
            propagateComposedEvents: true
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            anchors.fill: parent

            onClicked: mouse => {
                containersView.forceActiveFocus();
                mouse.accepted = false;
            }

            onPressed: mouse => {
                containersView.forceActiveFocus();
                mouse.accepted = false;
            }
        }

        focus : true
        activeFocusOnTab: true

        // Keys.onPressed: event => {
        //     console.log("onPressed key");
        //     event.accepted = true;
        // }

        // Disables swiping
        interactive: false

        visible : model.size > 0

        Layout.alignment : Qt.AlignTop

        Layout.fillWidth : true
        Layout.minimumWidth : fixedWidth
        Layout.maximumWidth : fixedWidth

        width : 100
        implicitHeight : contentItem.height;

        // reuseItems : true

        clip : true

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