import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import AppComponents as VMEComponent
import VME.dataModel 1.0

VMEComponent.ItemPanel {
    x: Math.random() * (1200 / 2 - 100)
    y: Math.random() * (800 - 100)

    // + 4 for the border and + 17 for the scrollbar
    implicitWidth: 8 * 32 + 4 + 17
    implicitHeight: 12 * 32

    color: "black"

    ColumnLayout {
        Layout.fillWidth: true
        Layout.fillHeight: true

        spacing: 0

        // https://doc.qt.io/qt-6/qtquickcontrols2-customize.html#customizing-combobox
        ComboBox {
            id: paletteDropdown
            textRole: "text"
            valueRole: "value"

            Layout.fillWidth: true

            model: VMEComponent.ComboBoxModel {
                id: paletteDropdownModel
                onModelReset: () => {}

                onSelectedIndexChanged: index => {
                    console.log(`[QML] Palette onSelectedIndexChanged: ${index}`);
                    paletteDropdown.currentIndex = index;
                }

                Component.onCompleted: () => {
                    AppDataModel.initializeItemPalettePanel(paletteDropdownModel, tilesetDropdownModel, brushList, tilesetModel);
                    paletteDropdown.currentIndex = 0;
                }

                Component.onDestruction: () => {
                    if (AppDataModel?.destroyItemPalettePanel) {
                        AppDataModel.destroyItemPalettePanel(paletteDropdownModel);
                    }
                }
            }

            onCurrentValueChanged: () => {
                // console.log(`paletteDropdown: ${currentValue}`);
                if (currentValue != '') {
                    tilesetDropdown.currentIndex = -1;
                    AppDataModel.setItemPalette(tilesetDropdownModel, currentValue);
                    tilesetDropdown.currentIndex = 0;
                }
            }

        }

        ComboBox {
            id: tilesetDropdown
            textRole: "text"
            valueRole: "value"

            Layout.fillWidth: true

            model: VMEComponent.ComboBoxModel {
                id: tilesetDropdownModel

                onSelectedIndexChanged: index => {
                    console.log(`[QML] Tileset onSelectedIndexChanged: ${index}`);
                    tilesetDropdown.currentIndex = index;
                }

                onModelReset: () => {
                    if (tilesetDropdownModel.rowCount() == 0) {
                        tilesetDropdown.currentIndex = -1;
                        tilesetModel.clear();
                        return;
                    }
                }
            }

            onCurrentValueChanged: () => {
                if (currentValue === undefined && currentIndex != -1) {
                    currentIndex = -1;
                    tilesetModel.clear();
                    return;
                }
                // console.log(`tilesetDropdown: ${currentValue}`);
                AppDataModel.setTileset(tilesetModel, paletteDropdown.currentValue, currentValue);
            }
        }

        VMEComponent.ThingList {
            id: brushList
            model: VMEComponent.TileSetModel {
                id: tilesetModel

                onBrushIndexSelected: index => {
                    console.log(`[QML] onBrushIndexSelected: ${index}`);
                    brushList.positionViewAtIndex(index);
                }
            }

            Layout.fillWidth: true
            Layout.fillHeight: true
        }
    }
}
