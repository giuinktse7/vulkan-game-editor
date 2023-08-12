import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import AppComponents as VMEComponent
import VME.dataModel 1.0

VMEComponent.ItemPanel {
    property var townModel: AppDataModel.townListModel
    x: 400
    y: 200

    implicitWidth: 300
    implicitHeight: 350

    color: "white"

    Component {
        id: testDelegate
        Rectangle {
            border.color: "purple"
            border.width: 1
            width: list.width
            height: 30

            MouseArea {
                anchors.fill: parent
                onClicked: () => {
                    console.log(`clicked on ${name} at index ${index} with itemId ${itemId}`);
                    list.currentIndex = index;
                }
            }

            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: `${name} (${templeX}, ${templeY}, ${templeZ})`
            }
        }
    }

    Rectangle {
        Layout.fillWidth: true
        Layout.fillHeight: true

        ColumnLayout {
            anchors.fill: parent
            spacing: 6

            Rectangle {
                Layout.alignment: Qt.AlignTop
                Layout.fillWidth: true
                Layout.minimumHeight: 100
                Layout.maximumHeight: 100

                // Layout.preferredWidth: childrenRect.width
                Layout.preferredHeight: list.height

                border {
                    color: "#B3E5FC"
                    width: 1
                }

                ListView {
                    id: list
                    anchors.fill: parent

                    // readonly property var currentTown: list.model.get(list.currentIndex)
                    Layout.preferredHeight: 150
                    Layout.maximumHeight: 200

                    highlight: Rectangle {
                        color: "lightsteelblue"
                        radius: 5
                    }

                    flickDeceleration: 10000
                    boundsMovement: Flickable.StopAtBounds
                    boundsBehavior: Flickable.StopAtBounds

                    ScrollBar.vertical: ScrollBar {
                        policy: ScrollBar.AlwaysOn
                        active: hovered || pressed
                        Layout.fillHeight: true
                    }

                    clip: true
                    focus: true

                    model: townModel
                    delegate: testDelegate

                    currentIndex: 0

                    onCountChanged: {
                        console.log(`onCountChanged - count: ${count}`);
                        if (count == 1) {
                            currentIndex = 0;
                        } else if (count == 0) {
                            currentIndex = -1;
                        }
                    }

                    onCurrentIndexChanged: {
                        if (currentIndex != -1) {
                            console.log(`currentIndexChanged: ${currentIndex}, town name: ${list.model.get(list.currentIndex)?.name}`);
                            const currentTown = list.model?.get(list.currentIndex);
                            townNameInput.text = currentTown?.name ?? "";
                            templePosXInput.text = currentTown?.x ?? "";
                            templePosYInput.text = currentTown?.y ?? "";
                            templePosZInput.text = currentTown?.z ?? "";
                        }
                    }
                }

                // Binding {
                //     target: townNameInput
                //     property: "text"
                //     value:
                // }

                // Binding {
                //     target: tabRoot
                //     property: "currentIndex"
                //     value: AppDataModel.currentMapIndex
                // }
            }

            VMEComponent.BorderedLayout {
                Layout.alignment: Qt.AlignTop
                Layout.fillWidth: true

                ColumnLayout {
                    spacing: 6

                    Text {
                        text: "ID: 5"
                    }

                    VMEComponent.InputField {
                        id: townNameInput
                        Layout.alignment: Qt.AlignTop
                        label: "Name"
                        text: list.model?.get(list.currentIndex)?.name ?? ""
                        Layout.preferredWidth: childrenRect.width
                        Layout.preferredHeight: childrenRect.height

                        onTextChanged: {
                            const index = list.currentIndex;
                            if (index != -1) {
                                const currentTown = list.model.get(index);
                                if (currentTown.name != text) {
                                    currentTown.name = text;
                                    list.model.nameChanged(text, index);
                                }
                            }
                        }
                    }

                    RowLayout {
                        Layout.alignment: Qt.AlignTop
                        VMEComponent.InputField {
                            id: templePosXInput
                            label: "X"
                            text: list.model?.get(list.currentIndex)?.x ?? ""
                            minimumInputWidth: 50
                            Layout.preferredWidth: childrenRect.width
                            Layout.preferredHeight: childrenRect.height

                            onTextChanged: {
                                const index = list.currentIndex;
                                if (index != -1) {
                                    const currentTown = list.model.get(index);
                                    if (currentTown.x.toString() != text) {
                                        const newValue = parseInt(text);
                                        currentTown.x = newValue;
                                        list.model.xChanged(newValue, index);
                                    }
                                }
                            }
                        }

                        VMEComponent.InputField {
                            id: templePosYInput
                            label: "Y"
                            text: "1000"
                            minimumInputWidth: 50
                            Layout.preferredWidth: childrenRect.width
                            Layout.preferredHeight: childrenRect.height

                            onTextChanged: {
                                const index = list.currentIndex;
                                if (index != -1) {
                                    const currentTown = list.model.get(index);
                                    if (currentTown.y.toString() != text) {
                                        const newValue = parseInt(text);
                                        currentTown.y = newValue;
                                        list.model.yChanged(newValue, index);
                                    }
                                }
                            }
                        }

                        VMEComponent.InputField {
                            id: templePosZInput
                            label: "Z"
                            text: "7"
                            minimumInputWidth: 50
                            Layout.preferredWidth: childrenRect.width
                            Layout.preferredHeight: childrenRect.height

                            onTextChanged: {
                                const index = list.currentIndex;
                                if (index != -1) {
                                    const currentTown = list.model.get(index);
                                    if (currentTown.z.toString() != text) {
                                        const newValue = parseInt(text);
                                        currentTown.z = newValue;
                                        list.model.zChanged(newValue, index);
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // TextField {
            //     id: textField
            //     placeholderText: "Enter a town name"
            //     width: 200
            //     font.pixelSize: 20
            //     onAccepted: {
            //         // Update the userInput property when the user submits their input
            //         userInput = text;
            //     }
            // }
            Button {
                Layout.alignment: Qt.AlignTop
                text: "New"
                onClicked: AppDataModel.createTown()
            }
        }
    }
}
