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
    implicitHeight: 200

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
                    list.currentIndex = index;
                }
            }

            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: "name: " + name
            }
        }
    }

    Rectangle {
        Layout.fillWidth: true
        Layout.fillHeight: true

        ColumnLayout {
            spacing: 6

            Rectangle {
                Layout.alignment: Qt.AlignTop
                Layout.fillWidth: true
                Layout.minimumHeight: 100
                Layout.maximumHeight: 100

                Layout.preferredWidth: childrenRect.width
                Layout.preferredHeight: list.height

                border {
                    color: "#B3E5FC"
                    width: 1
                }

                ListView {
                    id: list
                    anchors.fill: parent

                    // readonly property var currentTown: list.model.get(list.currentIndex)
                    Layout.maximumHeight: 100

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
                        if (count == 1) {
                            currentIndex = 0;
                        } else if (count == 0) {
                            currentIndex = -1;
                        }
                    }

                    onCurrentIndexChanged: {
                        if (currentIndex != -1) {
                            townNameInput.text = list.model?.get(list.currentIndex)?.name ?? "";
                        }
                    }
                }
            }

            VMEComponent.BorderedLayout {
                Layout.alignment: Qt.AlignTop

                ColumnLayout {
                    spacing: 6

                    Text {
                        text: "ID: 5"
                    }

                    VMEComponent.InputField {
                        id: townNameInput
                        label: "Name"
                        text: list.model?.get(list.currentIndex)?.name ?? ""
                        Layout.preferredWidth: childrenRect.width
                        Layout.preferredHeight: childrenRect.height

                        onTextChanged: {
                            if (list.currentIndex != -1) {
                                const currentTown = list.model.get(list.currentIndex);
                                if (currentTown.name != townNameInput.text) {
                                    currentTown.name = townNameInput.text;
                                    list.model.nameChanged(townNameInput.text, list.currentIndex);
                                }
                            }
                        }
                    }

                    RowLayout {
                        VMEComponent.InputField {
                            id: templePosXInput
                            label: "X"
                            text: "1000"
                            minimumInputWidth: 50
                            Layout.preferredWidth: childrenRect.width
                            Layout.preferredHeight: childrenRect.height
                        }

                        VMEComponent.InputField {
                            id: templePosYInput
                            label: "Y"
                            text: "1000"
                            minimumInputWidth: 50
                            Layout.preferredWidth: childrenRect.width
                            Layout.preferredHeight: childrenRect.height
                        }
                        VMEComponent.InputField {
                            id: templePosZInput
                            label: "Z"
                            text: "7"
                            minimumInputWidth: 50
                            Layout.preferredWidth: childrenRect.width
                            Layout.preferredHeight: childrenRect.height
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
