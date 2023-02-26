import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import AppComponents as VMEComponent

VMEComponent.ItemPanel {
    property alias model: itemList.model;

    x: Math.random() * (1200 / 2 - 100);
    y: Math.random() * (800 - 100);

    color: "black";

    ColumnLayout {
        Layout.fillWidth: true;
        Layout.fillHeight: true;

        ComboBox {
            textRole: "text"
            valueRole: "value"

            Layout.fillWidth: true;

            model: VMEComponent.ItemPaletteModel { }
        }

        VMEComponent.ThingList {
            id: itemList;
            Layout.fillWidth: true;
            Layout.fillHeight: true;
        }    
    }
}