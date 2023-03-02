import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import AppComponents as VMEComponent
import VME.dataModel 1.0

VMEComponent.ItemPanel {
    x: Math.random() * (1200 / 2 - 100)
    y: Math.random() * (800 - 100)

    implicitWidth: 8 * 32
    implicitHeight: 12 * 32

    color: "black"

    ColumnLayout {
        Layout.fillWidth: true
        Layout.fillHeight: true

        // https://doc.qt.io/qt-6/qtquickcontrols2-customize.html#customizing-combobox
        ComboBox {
            id: paletteDropdown
            textRole: "text"
            valueRole: "value"

            Layout.fillWidth: true

            model: VMEComponent.ComboBoxModel {
                id: paletteListModel
                onModelReset: () => {}

                Component.onCompleted: () => {
                    AppDataModel.setItemPaletteList(paletteListModel);
                    paletteDropdown.currentIndex = 0;
                    // console.log(`Completed. ${paletteDropdown.currentIndex}: ${paletteDropdown.currentText}, ${paletteDropdown.currentValue}`);
                    // if (paletteDropdown.currentValue != '') {
                    // AppDataModel.setItemPalette(tilesetModel, paletteDropdown.currentValue);
                    // }
                }
            }

            onCurrentValueChanged: () => {
                // console.log(`paletteDropdown: ${currentValue}`);
                if (currentValue != '') {
                    tilesetDropdown.currentIndex = -1;
                    AppDataModel.setItemPalette(tilesetModel, currentValue);
                    tilesetDropdown.currentIndex = 0;
                }
            }

            // Connections {
            //     target: parent.model
            //     function onDataChanged() {
            //         console.log("hoho");
            //     }
            // }
        }

        ComboBox {
            id: tilesetDropdown
            textRole: "text"
            valueRole: "value"

            Layout.fillWidth: true

            model: VMEComponent.ComboBoxModel {
                id: tilesetModel

                onModelReset: () => {
                    if (tilesetModel.rowCount() == 0) {
                        tilesetDropdown.currentIndex = -1;
                        brushModel.clear();
                        return;
                    }
                }
            }

            onCurrentValueChanged: () => {
                if (currentValue === undefined && currentIndex != -1) {
                    currentIndex = -1;
                    brushModel.clear();
                    return;
                }
                // console.log(`tilesetDropdown: ${currentValue}`);
                AppDataModel.setTileset(brushModel, paletteDropdown.currentValue, currentValue);
            }
        }

        VMEComponent.ThingList {
            id: itemList
            model: VMEComponent.TileSetModel {
                id: brushModel
            }

            Layout.fillWidth: true
            Layout.fillHeight: true
        }
    }
}
