import QtQuick.Controls
import QtQuick
import QtQuick.Layouts
import "./vme" as Vme
import Vme.context 1.0 as Context

Rectangle {
    id: root
    anchors.fill: parent;
    anchors.margins: 14

    property var containers;
    property var fluidTypeModel;
    
    property bool isStackable: false
    property bool isFluid: false
    property bool isWriteable: false

    // border.color: "red"
    // border.width: 2

    Column {
        id: mainContainer
        width: parent.width;
        spacing: 14

        Rectangle {
            width: parent.width;
            height: topSegment.height;

            border.color: "blue"
            border.width: 1
            Column {
                width: parent.width;
                id: topSegment
                
                Image {
                    objectName : "property_item_image"
                    width: 32
                    height: 32
                    source: ""
                }

                RowLayout {
                    width: parent.width

                    Vme.IntItemProperty {
                        Layout.alignment: Qt.AlignHCenter

                        inputWidth: parent.width * 0.4
                        label: "Action ID"
                        inputObjectName: "item_actionid_input"
                        from: 0
                        to: 65535

                        onValueChanged: {
                            if (visible) {
                                // Context.C_PropertyWindow.setPropertyItemActionId(value);
                            }
                        }

                        onEditingFinished: value => {
                            // Context.C_PropertyWindow.setPropertyItemActionId(value, true);
                        }
                    }

                    Vme.IntItemProperty {
                        Layout.alignment: Qt.AlignHCenter

                        inputWidth: parent.width * 0.4
                        label: "Unique ID"
                        inputObjectName: "item_uniqueid_input"
                        from: 0
                        to: 65535
                    }
                }
            }
        }

        Rectangle {
            width: parent.width;
            height: 1;
            color: "#BDBDBD";
        }

        Rectangle {
            width: parent.width;
            height: middleSegment.height

            border.color: "green"

            Column {
                id: middleSegment
                width: root.width;
                height: root.height - (topSegment.height + bottomSegment.height) - mainContainer.spacing * 3

                Vme.IntItemProperty {
                    id: countProperty

                    visible: root.isStackable
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
    
                Column {
                    visible: root.isFluid
                    spacing: 4

                    Text {
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
                        model: root.fluidTypeModel;

                        objectName: "item_fluid_type_input"

                        onActivated: {
                            Context.C_PropertyWindow.setFluidType(currentIndex);
                        }

                        onHighlighted: {
                            Context.C_PropertyWindow.fluidTypeHighlighted(highlightedIndex);
                        }
                    }
                }

                Item {
                    id: itemTextWrapper
                    anchors.fill: parent

                    visible: root.isWriteable

                    Rectangle {
                        anchors.fill: parent;
                        border.color: "#ccc"
                        border.width: 1

                        ScrollView {
                            anchors.fill: parent
                            anchors.margins: 1

                            TextArea {
                                objectName : "item_text_input"

                                // anchors.fill: parent
                                // width: parent.parent.width
                                id: itemText
                                text: ""
                                wrapMode: TextEdit.WrapAnywhere

                                selectByMouse: true
                                focus: true

                                onEditingFinished: {
                                    Context.C_PropertyWindow.setPropertyItemText(text);
                                }
                            }
                        }
                        
                    }
                }     
            }
        }

        Rectangle {
            width: parent.width;
            height: bottomSegment.height

            border.color: "red"

            Column {
                id: bottomSegment;
                width: root.width;
                height: root.height / 2  - (root.anchors.margins * 2)

                ListView {
                    id: containersView
                    objectName: "item_container_area"

                    ScrollBar.vertical: ScrollBar {}
                    ScrollIndicator.vertical: ScrollIndicator { }

                     MouseArea {
                        anchors.fill: parent
                        acceptedButtons: Qt.NoButton

                        hoverEnabled: false
                        propagateComposedEvents: true
                        onWheel: e => {
                            const scrollBar = containersView.ScrollBar.vertical
                            const newPos = scrollBar.position - Math.sign(e.angleDelta.y) * 0.1;

                            scrollBar.position = Math.max(0, Math.min(newPos, 1 - scrollBar.size));
                            e.accepted = true;
                        }
                    }


                    anchors.horizontalCenter: parent.horizontalCenter

                    readonly property int fixedWidth: 36 * 4

                    visible: model.size > 0
                    model: root.containers;
                    focus: true
                    activeFocusOnTab: true
                    width: fixedWidth
                    // implicitHeight: contentItem.height;
                    implicitHeight: parent.height;
                    clip: true
                    

                    // Disables swiping
                    interactive: false

                    // Layout.alignment : Qt.AlignTop

                    // Layout.fillWidth: true
                    // Layout.fillHeight: true
                    // Layout.minimumWidth: fixedWidth
                    // Layout.maximumWidth: fixedWidth

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
    }
}

// ScrollView {
//     id: root
//     anchors.top: parent.top
//     anchors.left: parent.left
//     anchors.right: parent.right
//     anchors.margins: 14

//     property bool isStackable: false
//     property bool isFluid: false
//     property bool isWriteable: false

//     visible: false
//     clip: true
//     // padding: 14

//     property var containers
//     property var fluidTypeModel

//     // Required to make things have correct width
//     Rectangle {
//         anchors.fill: parent
//         border.width: 0
//     }

//     ColumnLayout {
//         id: contents
//         anchors.fill: parent

//         ColumnLayout {
//             Layout.alignment: Qt.AlignTop
//             Layout.bottomMargin: 8;

//             Image {
//                 objectName : "property_item_image"
//                 width: 32
//                 height: 32
//                 source: ""
//             }
//         }

//         Vme.IntItemProperty {
//             Layout.alignment: Qt.AlignTop;
//             Layout.bottomMargin: 8;

//             label: "Action ID"
//             inputObjectName: "item_actionid_input"
//             from: 0
//             to: 65535

//             onValueChanged: {
//                 console.log("Here: ", contents.width)
//                 if (visible) {
//                     Context.C_PropertyWindow.setPropertyItemActionId(value);
//                 }
//             }

//             onEditingFinished: value => {
//                 Context.C_PropertyWindow.setPropertyItemActionId(value, true);
//             }
//         }

//         Vme.IntItemProperty {
//             Layout.alignment: Qt.AlignTop;
//             Layout.bottomMargin: 8;

//             label: "Unique ID"
//             inputObjectName: "item_uniqueid_input"
//             from: 0
//             to: 65535
//         }

//         Vme.IntItemProperty {
//             id: countProperty
//             Layout.alignment: Qt.AlignTop;
//             Layout.bottomMargin: 8;

//             visible: root.isStackable
//             label: "Count"
//             inputObjectName: "item_count_input"
//             from: 1
//             to: 100
            
//             onValueChanged: {
//                 if (visible) {
//                     Context.C_PropertyWindow.setPropertyItemCount(value);
//                 }
//             }

//             onEditingFinished: value => {
//                 Context.C_PropertyWindow.setPropertyItemCount(value, true);
//             }
//         }
    
//         Column {
//             visible: root.isFluid
//             spacing: 4

//             Layout.alignment: Qt.AlignTop;
//             Layout.bottomMargin: 8;
//             Layout.preferredWidth: parent.width

//             Text {
//                 text : "Fluid"
//                 color : "#565e65"

//                 font {
//                     pointSize : 7
//                     family : Vme.Constants.labelFontFamily
//                     capitalization : Font.AllUppercase
//                 }
//             }

//             Vme.Dropdown {
//                 id: fluidProperty
//                 model: root.fluidTypeModel;

//                 objectName: "item_fluid_type_input"

//                 onActivated: {
//                     Context.C_PropertyWindow.setFluidType(currentIndex);
//                 }

//                 onHighlighted: {
//                     Context.C_PropertyWindow.fluidTypeHighlighted(highlightedIndex);
//                 }
//             }
//         }

//         Item {
//             id: itemTextWrapper
//             Layout.alignment: Qt.AlignTop;
//             Layout.bottomMargin: 8;
//             Layout.fillWidth: true
//             height: 200

//             visible: root.isWriteable

//             Rectangle {
//                 width: parent.width / 2;
//                 anchors.top: parent.top;
//                 anchors.bottom: parent.bottom;
//                 border.color: "#ccc"
//                 border.width: 1

//                 ScrollView {
//                     anchors.fill: parent
//                     anchors.margins: 1

//                     TextArea {
//                         objectName : "item_text_input"

//                         anchors.fill: parent
//                         // width: parent.parent.width
//                         id: itemText
//                         text: ""
//                         wrapMode: TextEdit.WrapAnywhere

//                         selectByMouse: true
//                         focus: true

//                         onEditingFinished: {
//                             Context.C_PropertyWindow.setPropertyItemText(text);
//                         }
//                     }
//                 }
                
//             }
//         }
       
//         ListView {
//             id: containersView
//             objectName: "item_container_area"

//             readonly property int fixedWidth: 36 * 4

//             visible: model.size > 0
//             model: root.containers;
//             focus: true
//             activeFocusOnTab: true
//             width: 100
//             implicitHeight: contentItem.height;
//             clip: true

//             // Disables swiping
//             interactive: false

//             Layout.alignment : Qt.AlignTop

//             Layout.fillWidth: true
//             Layout.fillHeight: true
//             Layout.minimumWidth: fixedWidth
//             Layout.maximumWidth: fixedWidth

//             MouseArea {
//                 propagateComposedEvents: true
//                 acceptedButtons: Qt.LeftButton | Qt.RightButton
//                 anchors.fill: parent

//                 onClicked: mouse => {
//                     containersView.forceActiveFocus();
//                     mouse.accepted = false;
//                 }

//                 onPressed: mouse => {
//                     containersView.forceActiveFocus();
//                     mouse.accepted = false;
//                 }
//             }

//             delegate : Component {
//                 Vme.ItemContainerWindow {
//                     id : itemContainerView

//                     required property var itemModel

//                     model : itemModel
                
//                     onClose: () => {
//                     containersView.model.closeContainer(index);
//                     }
//                 }
//             }
//         }

//   }
// }