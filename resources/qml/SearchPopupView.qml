import QtQuick.Controls 2.15
import QtQuick 2.15
import QtQuick.Layouts
import "./vme" as Vme
import "./vme/search_popup" as SearchPopup
import Vme.context 1.0 as Context

Rectangle {
    id: root;
    anchors.fill: parent;
    border.color: "#aaa"
    border.width: 1

    property int minHeight: 300;
    property int maxHeight: 600;

    property var searchResults;

    color: "transparent";

    function resetSearchText() {
        search_textfield.text = "";
    }

    
    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton

        id: mouseArea
        property int oldMouseY
        property int startHeight
        property bool resizing : false;
        cursorShape: containsMouse
                     || pressed ? Qt.SizeVerCursor : Qt.ArrowCursor
        preventStealing: true
        propagateComposedEvents: true

        // Rectangle {
        //   anchors.fill: parent
        //   color: "red"
        // }

        hoverEnabled: true

        onPressed: (mouse) => {
            if ((mouseY > height - 5)) {
                oldMouseY = mouseY
                startHeight = root.height
                resizing = true;
                applicationContext.setCursor(Qt.SizeVerCursor)
            } else {
                mouse.accepted = false;
            }
        }
        
        onReleased: {
            if (resizing) {
                applicationContext.resetCursor()
                resizing = false;
            }
        }

        onPositionChanged: {
            if (!pressed && resizing) {
                applicationContext.resetCursor()
                resizing = false;
            }

            cursorShape = ((mouseY > height - 5) || resizing) ? Qt.SizeVerCursor : Qt.ArrowCursor;

            if (resizing) {
                const deltaY = (mouseY - oldMouseY)
                const newHeight = Math.max(root.minHeight,
                                       Math.min(startHeight + deltaY,
                                                root.maxHeight))
                Context.C_SearchPopupView.setHeight(newHeight);
            }
        }
    }


    // ShaderEffectSource {
    //     id: theSource
    //     sourceItem: test_rect
    // }

    // ShaderEffect {
    //     anchors.fill: parent;
    //     fragmentShader: "qrc:/test.frag.qsb"
    // }

    Rectangle {
        anchors.fill: parent;
        // anchors.margins: 14;
            
        color: "#fcfcfc";
        // color: "#ccc";

        ColumnLayout {
            anchors.fill: parent;
            TextField {
                id: search_textfield
                Layout.alignment : Qt.AlignTop
                Layout.preferredHeight: 30;
                Layout.leftMargin: 7;
                Layout.topMargin: 10;
                objectName: "search_textfield"

                selectByMouse: true;
                color: "#222222"
                background: Item {}
                placeholderText: qsTr("Search for brushes...");
                font.pixelSize: 12;

                onTextChanged: {
                    Context.C_SearchPopupView.searchEvent(text);
                }
            }

            Rectangle {
                Layout.margins: 0;
                Layout.fillWidth: true;
                height: 1;
                color: "#BDBDBD";
            }

            Row {
                id: filterOptionsRow
                spacing: 30;
                Layout.fillWidth: true;
                Layout.alignment : Qt.AlignTop;
                Layout.leftMargin: 15;
                Layout.rightMargin: 15;
                Layout.bottomMargin: 7;
                Layout.topMargin: 7;

                readonly property int fontSize: 11;
                property string selectedId;
                property int selectedOffsetX;
                property int selectedWidth;

                onSelectedIdChanged: {
                    filter_selection_indicator.x = selectedOffsetX;
                    filter_selection_indicator.width = selectedWidth + spacing;
                }

                SearchPopup.FilterChoice {
                    readonly property int filterId: 0;
                    id: "filter_choice_all"
                    text: qsTr("All");
                    fontSize: filterOptionsRow.fontSize;
                    selected: filterOptionsRow.selectedId == filterId;
                    amount: root.searchResults.totalCount;
                    onPressed: {
                        root.searchResults.resetFilter();
                        filterOptionsRow.selectedWidth = width;
                        filterOptionsRow.selectedOffsetX = x;
                        filterOptionsRow.selectedId = filterId;
                    }
                }
                SearchPopup.FilterChoice {
                    readonly property int filterId: 1;
                    id: "filter_choice_raw"
                    text: qsTr("Raw");
                    fontSize: filterOptionsRow.fontSize;
                    selected: filterOptionsRow.selectedId == filterId;
                    amount: root.searchResults.rawCount;
                    onPressed: {
                        root.searchResults.setFilter("raw");

                        filterOptionsRow.selectedWidth = width;
                        filterOptionsRow.selectedOffsetX = x;
                        filterOptionsRow.selectedId = filterId;
                    }
                }
                SearchPopup.FilterChoice {
                    readonly property int filterId: 2;
                    id: "filter_choice_ground"
                    text: qsTr("Ground");
                    fontSize: filterOptionsRow.fontSize;
                    selected: filterOptionsRow.selectedId == filterId;
                    amount: root.searchResults.groundCount;
                    onPressed: {
                        root.searchResults.setFilter("ground");

                        filterOptionsRow.selectedWidth = width;
                        filterOptionsRow.selectedOffsetX = x;
                        filterOptionsRow.selectedId = filterId;
                    }
                }
                SearchPopup.FilterChoice {
                    readonly property int filterId: 3;
                    id: "filter_choice_doodad"
                    text: qsTr("Doodad");
                    fontSize: filterOptionsRow.fontSize;
                    selected: filterOptionsRow.selectedId == filterId;
                    amount: root.searchResults.doodadCount;
                    onPressed: {
                        root.searchResults.setFilter("doodad");

                        filterOptionsRow.selectedWidth = width;
                        filterOptionsRow.selectedOffsetX = x;
                        filterOptionsRow.selectedId = filterId;
                    }
                }
            }

            Item {
                Layout.fillWidth: true;
                Rectangle {
                    width: parent.width;
                    height: 1;
                    color: "#BDBDBD";
                }

                Rectangle {
                    id: "filter_selection_indicator";
                    width: 50;
                    height: 2;
                    color: "#4F4F4F";

                    Behavior on x {
                        NumberAnimation { easing.type: Easing.InOutQuad; duration: 150 }
                    }

                    Behavior on width {
                        NumberAnimation { easing.type: Easing.InOutQuad; duration: 150 }
                    }
                }
            }
            


            
            GridView {
                id: searchResultList

                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.margins: 30;

                model: root.searchResults;

                cellWidth: 85
                cellHeight: 85

                readonly property int cellHSpacing: 14;
                readonly property int cellVSpacing: 14;

                focus: true
                clip: true
                interactive: false

                ScrollBar.vertical: ScrollBar {
                    interactive: true
                }
                // ScrollIndicator.vertical: ScrollIndicator { }

                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.NoButton

                    hoverEnabled: false
                    propagateComposedEvents: true
                    onWheel: e => {
                        const scrollBar = searchResultList.ScrollBar.vertical
                        const newPos = scrollBar.position - Math.sign(e.angleDelta.y) * 0.06;

                        scrollBar.position = Math.max(0, Math.min(newPos, 1 - scrollBar.size));
                        e.accepted = true;
                    }
                }

                
                delegate: Component {
                    id: itemDelegate

                    MouseArea {
                        id: brush

                        required property string name
                        required property int serverId
                        required property int index

                        width: childrenRect.width;
                        height: childrenRect.height;

                        hoverEnabled: true

                        onPressed: {
                            Context.C_SearchPopupView.brushSelected(index);
                        }

                        // ToolTip {
                        //     id: nameTextTooltip;
                        //     parent: nameText;
                        //     visible: nameText.truncated && nameTextMouseArea.containsMouse
                        //     text: brush.name
                        //     delay: 300
                        //     y: -5;

                        //     contentItem: MouseArea {
                        //         width: childrenRect.width;
                        //         height: childrenRect.height;

                        //         onPressed: {
                        //             Context.C_SearchPopupView.brushSelected(brush.index);
                        //         }
                        //         Text {
                        //             text: nameTextTooltip.text;
                        //             font: nameTextTooltip.font;
                        //             color: "white";
                        //         }
                        //     }

                        //     background: Rectangle {
                        //         color: "#CC303F9F"
                        //     }
                        // }

                        Column {
                            width: searchResultList.cellWidth - searchResultList.cellHSpacing;
                            height: searchResultList.cellHeight - searchResultList.cellVSpacing;

                            Rectangle {
                                anchors.horizontalCenter: parent.horizontalCenter;
                                width: 32;
                                height: 32;
                                // Layout.preferredWidth: 32;
                                // Layout.preferredHeight: 32;
                                // Layout.alignment: Qt.AlignHCenter | Qt.AlignTop;

                                color: "transparent";

                                border.color: "#f2f2f2";
                                // border.color: "red";
                                border.width: 1;

                                Image {
                                anchors.centerIn: parent
                                    source: {
                                        return "image://itemTypes/" + brush.serverId;
                                    }
                                }
                            }


                            // Column {
                            //     width: searchResultList.cellWidth - searchResultList.cellHSpacing;
                            //     height: childrenRect.height
                                // Layout.preferredWidth: 32
                                // Layout.preferredHeight: childrenRect.height
                                // Layout.alignment: Qt.AlignHCenter | Qt.AlignTop


                                Text {
                                    width: parent.width
                                    text: brush.serverId
                                    horizontalAlignment: Text.AlignHCenter;

                                }
                                Text {
                                    id: nameText
                                    width: parent.width
                                    text: brush.name
                                    wrapMode: Text.WordWrap
                                    elide: Text.ElideRight
                                    maximumLineCount: brush.containsMouse ? 3 : 1;

                                    horizontalAlignment: Text.AlignHCenter;

                                    MouseArea {
                                        id: nameTextMouseArea
                                        anchors.fill: parent;
                                        hoverEnabled: true
                                    }
                                }

                            
                            // }
                        }
                    }
                }
            }

        }
    }
}