import QtQuick.Controls
import QtQuick
import QtQuick.Layouts
import AppComponents as VMEComponent
import VME.dataModel 1.0
import VME.minimap 1.0

VMEComponent.ItemPanel {
    id: root

    z: 3

    closeClicked: () => {
        close();
    }

    function open() {
        root.visible = true;
        root.enabled = true;
        Minimap.open = true;
    }

    function close() {
        root.visible = false;
        root.enabled = false;
        Minimap.open = false;
    }

    // anchors.fill: parent
    implicitWidth: 300
    implicitHeight: 300

    x: 600
    y: 250

    property int minHeight: 300
    property int maxHeight: 600

    color: "#BDBDBD"

    Rectangle {
        Layout.fillWidth: true
        Layout.fillHeight: true

        color: "#fcfcfc"

        VMEComponent.RefreshableImage {
            id: item
            anchors.fill: parent

            image: Minimap.image

            onWidthChanged: () => {
                Minimap.setWidth(width);
            }

            onHeightChanged: () => {
                Minimap.setHeight(height);
            }

            MouseArea {
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                anchors.fill: parent
                propagateComposedEvents: true

                onPressed: () => {
                    // TODO
                    console.log("Minimap clicked");
                }
            }
        }
    }
}
