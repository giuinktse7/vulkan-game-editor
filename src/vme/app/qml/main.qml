import app
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import AppComponents as VMEComponent

Item {
    id: root;
    width: 800;
    height: 600;

    property var editor;
    property var tilesetModel;


    // MapViewItem {
    //     anchors.fill: parent
    //     focus: true
    
    //     Keys.onPressed: (event) => { console.log("pressed " + event.key); }
    // }

    StackLayout {
        id: mapAreaStackLayout
        anchors.fill: parent
        anchors.margins: 0
        currentIndex: tabRoot.currentIndex
        property var prevIndex: 0
        
        onCurrentIndexChanged: (x) => {
            root.editor.mapTabSelected(prevIndex, currentIndex);
            prevIndex = currentIndex;
        }

        Repeater {
            model: root.editor.mapTabs
            delegate: VMEComponent.MapView {
                // Very important! Without these, resizing can fail
                anchors.fill: parent
                anchors.margins: 0
                
                required property string item
                required property int theId
                required property int index

                id: mapView

                Component.onCompleted: () => {
                    // console.log(`Index: ${index}`)
                    mapView.entryId = theId;
                    // console.log(mapView, mapView.entryId, item, theId)
                }
            }
        }
    }


    // Rectangle {
    //     color: "transparent"
    //     anchors.fill: parent

    //     ColumnLayout {
    //         spacing: 0

    //         Rectangle {
    //             Layout.fillWidth: true
    //             color: "red"
    //             Layout.preferredWidth: 40
    //             Layout.preferredHeight: 40
    //         }

    //         Rectangle {
    //             Layout.fillWidth: true
    //             color: "green"
    //             Layout.preferredWidth: 40
    //             Layout.preferredHeight: 70
    //         }

    //         Rectangle {
    //             Layout.fillWidth: true
    //             Layout.fillHeight: true
    //             color: "blue"
    //             Layout.preferredWidth: 70
    //             Layout.preferredHeight: 40
    //         }
    //     }
    // }

   GridLayout {
        id: grid;
        anchors.fill: parent;
        rowSpacing: 0;
        columnSpacing: 0

        rows: 3;
        columns: 3;

        VMEComponent.ItemPalette {
            color: "blue";
            model: root.tilesetModel;

            Layout.preferredWidth: rectWidth;
            Layout.fillHeight: true;
            Layout.row: 1;
            Layout.column: 0;
        }
 
        Rectangle {
            color: "green"
            Layout.preferredHeight: 80
            Layout.fillWidth: true
            Layout.columnSpan: 3
            Layout.row: 0
            Layout.column: 0
        }

        Rectangle {
            id: mapViewTabSection
            color: "#F5F5F5"
            Layout.preferredHeight: mapViewBar.height;

            Layout.row: 1
            Layout.column: 1
            Layout.alignment: Qt.AlignTop
            Layout.fillWidth: true

            RowLayout {
                id: mapViewBar

                TabBar {
                    id: tabRoot

                    onCurrentIndexChanged: () => {
                       root.editor.currentMapIndex = currentIndex;
                    }

                    Repeater {
                        model: root.editor.mapTabs
                        delegate: TabButton {
                            horizontalPadding: 8;
                            verticalPadding: 4;
                            text: item
                        }
                    }
                }

                // TabBar {
                //     TabButton {
                //         horizontalPadding: 8;
                //         verticalPadding: 4;
                //         text: "Untitled 1";
                //     }

                //     TabButton {
                //         horizontalPadding: 8;
                //         verticalPadding: 4;
                //         text: "Untitled 2"
                //     }

                //     TabButton {
                //         horizontalPadding: 8;
                //         verticalPadding: 4;
                //         text: "Untitled 3"
                //     }
                // }
            }

        }

        Rectangle {
            color: "purple"
            Layout.fillWidth: true
            Layout.preferredHeight: 30;
            Layout.columnSpan: 3
            Layout.row: 2
            Layout.column: 0
       }

        Rectangle {
            color: "green"
            Layout.fillHeight: true
            Layout.preferredWidth: 30;
            Layout.row: 1
            Layout.column: 2
       }
   }
}
