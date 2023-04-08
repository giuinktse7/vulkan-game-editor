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
                    console.log(`clicked on ${name} at index ${index} with itemId ${itemId}`);
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
            anchors.fill: parent
            spacing: 0

            ListView {
                id: list
                Layout.fillWidth: true
                Layout.minimumHeight: 100
                highlight: Rectangle {
                    color: "lightsteelblue"
                    radius: 5
                }
                focus: true

                readonly property var currentTown: list.currentIndex != -1 ? list.model.get(list.currentIndex) : undefined

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

                onCurrentTownChanged: {
                    if (currentTown) {
                        console.log(`currentTownChanged: ${currentTown.name}`);
                        textInput.text = currentTown.name;
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.minimumHeight: 30
                height: textInput.height
                color: red

                border {
                    color: "black"
                    width: 1
                }

                radius: 2

                TextInput {
                    id: textInput
                    anchors.centerIn: parent
                    font.pointSize: 12

                    // text: list.currentIndex != -1 ? list.currentItem.name : ""
                    padding: 10
                    selectionColor: "#6666FF"
                    cursorVisible: true
                    focus: true

                    onTextChanged: {
                        console.log("[JS] textInput::onTextChanged");
                        if (list.currentIndex != -1) {
                            console.log(`[JS] textInput::onTextChanged - list.currentIndex != -1, currenttown: ${list.currentTown}, name: ${list.currentTown.name}`);
                            // list.model.get(list.currentIndex).name = textInput.text;
                            if (list.currentTown) {
                                console.log(`Setting name to ${textInput.text}`);
                                list.currentTown.name = textInput.text;
                            }
                            list.model.textChanged(textInput.text, list.currentIndex);
                        }
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.minimumHeight: 30

                Rectangle {
                    Layout.minimumWidth: 80
                    Layout.minimumHeight: 30
                    TextInput {
                        anchors.verticalCenter: parent.verticalCenter
                        font.pointSize: 12
                        text: "1000"

                        validator: IntValidator {
                        }
                    }
                }

                Rectangle {
                    Layout.minimumWidth: 80
                    Layout.minimumHeight: 30
                    TextInput {
                        anchors.verticalCenter: parent.verticalCenter
                        font.pointSize: 12
                        text: "1000"

                        validator: IntValidator {
                        }
                    }
                }

                Rectangle {
                    Layout.minimumWidth: 80
                    Layout.minimumHeight: 30
                    TextInput {
                        anchors.verticalCenter: parent.verticalCenter
                        font.pointSize: 12
                        text: "7"

                        validator: IntValidator {
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
                text: "New"
                onClicked: AppDataModel.createTown()
            }
        }
    }
}
