import QtQuick.Controls 2.15
import QtQuick 2.15
import QtQuick.Layouts
import "./vme" as Vme
import "./vme/search_popup" as SearchPopup
import Vme.context 1.0 as Context

Rectangle {
    id: root;
    anchors.fill: parent;

    property int minHeight: 300;
    property int maxHeight: 600;

    property var searchResults;

    color: "#BDBDBD";

    function resetSearchText() {
        search_textfield.text = "";
    }

    function focusSearchTextInput() {
        search_textfield.forceActiveFocus();
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
        anchors.margins: 1;
            
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

               activeFocusOnTab: true;
               onActiveFocusChanged: {
               if (activeFocus) {
                        console.log("Focused search field");
                    }
                }

                onTextChanged: {
                    Context.C_SearchPopupView.searchEvent(text);
                }

                Keys.onDownPressed: {
                    if (root.searchResults.count != 0) {
                        searchResultList.itemAtIndex(0).forceActiveFocus();
                    }
                }

                onAccepted: {
                    if (root.searchResults.count != 0) {
                        Context.C_SearchPopupView.brushSelected(0);
                    }
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

                function updateSelectionIndicator(x, width) {
                    filter_selection_indicator.x = x;
                    filter_selection_indicator.width = width + spacing;
                }

                SearchPopup.FilterChoice {
                    readonly property int filterId: 0;
                    id: "filter_choice_all"
                    text: qsTr("All");
                    fontSize: filterOptionsRow.fontSize;
                    selected: filterOptionsRow.selectedId == filterId;
                    amount: root.searchResults.totalCount;

                    activeFocusOnTab: true;
                    onActiveFocusChanged: {
                        if (activeFocus) {
                            console.log("Focused ", filterId);
                        }
                    }

                    onPressed: {
                        root.searchResults.resetFilter();
                        filterOptionsRow.updateSelectionIndicator(x, width);
                        filterOptionsRow.selectedId = filterId;
                    }

                    onWidthChanged: {
                        if (selected) {
                            filterOptionsRow.updateSelectionIndicator(x, width);
                        }
                    }

                    onXChanged: {
                        if (selected) {
                            filterOptionsRow.updateSelectionIndicator(x, width);
                        }
                    }
                }

                SearchPopup.FilterChoice {
                
                    readonly property int filterId: 1;
                    id: "filter_choice_raw"
                    text: qsTr("Raw");
                    fontSize: filterOptionsRow.fontSize;
                    selected: filterOptionsRow.selectedId == filterId;
                    amount: root.searchResults.rawCount;

                    activeFocusOnTab: true;
                    onActiveFocusChanged: {
                        if (activeFocus) {
                            console.log("Focused ", filterId);
                        }
                    }

                    onPressed: {
                        root.searchResults.setFilter("raw");
                        filterOptionsRow.updateSelectionIndicator(x, width);
                        filterOptionsRow.selectedId = filterId;
                    }

                    onWidthChanged: {
                        if (selected) {
                            filterOptionsRow.updateSelectionIndicator(x, width);
                        }
                    }

                    onXChanged: {
                        if (selected) {
                            filterOptionsRow.updateSelectionIndicator(x, width);
                        }
                    }
                }

                SearchPopup.FilterChoice {
                    readonly property int filterId: 2;
                    id: "filter_choice_ground"
                    text: qsTr("Ground");
                    fontSize: filterOptionsRow.fontSize;
                    selected: filterOptionsRow.selectedId == filterId;
                    amount: root.searchResults.groundCount;

                    activeFocusOnTab: true;
                    onActiveFocusChanged: {
                        if (activeFocus) {
                            console.log("Focused ", filterId);
                        }
                    }

                    onPressed: {
                        root.searchResults.setFilter("ground");
                        filterOptionsRow.updateSelectionIndicator(x, width);
                        filterOptionsRow.selectedId = filterId;
                    }

                    onWidthChanged: {
                        if (selected) {
                            filterOptionsRow.updateSelectionIndicator(x, width);
                        }
                    }

                    onXChanged: {
                        if (selected) {
                            filterOptionsRow.updateSelectionIndicator(x, width);
                        }
                    }
                }

                SearchPopup.FilterChoice {
                    readonly property int filterId: 3;
                    id: "filter_choice_doodad"
                    text: qsTr("Doodad");
                    fontSize: filterOptionsRow.fontSize;
                    selected: filterOptionsRow.selectedId == filterId;
                    amount: root.searchResults.doodadCount;

                    activeFocusOnTab: true;
                    onActiveFocusChanged: {
                        if (activeFocus) {
                            console.log("Focused ", filterId);
                        }
                    }

                    onPressed: {
                        root.searchResults.setFilter("doodad");
                        filterOptionsRow.updateSelectionIndicator(x, width);
                        filterOptionsRow.selectedId = filterId;
                    }

                    onWidthChanged: {
                        if (selected) {
                            filterOptionsRow.updateSelectionIndicator(x, width);
                        }
                    }

                    onXChanged: {
                        if (selected) {
                            filterOptionsRow.updateSelectionIndicator(x, width);
                        }
                    }
                }

                SearchPopup.FilterChoice {
                    readonly property int filterId: 4;
                    id: "filter_choice_creature"
                    text: qsTr("Creature");
                    fontSize: filterOptionsRow.fontSize;
                    selected: filterOptionsRow.selectedId == filterId;
                    amount: root.searchResults.creatureCount;

                    activeFocusOnTab: true;
                    onActiveFocusChanged: {
                        if (activeFocus) {
                            console.log("Focused ", filterId);
                        }
                    }

                    onPressed: {
                        root.searchResults.setFilter("creature");
                        filterOptionsRow.updateSelectionIndicator(x, width);
                        filterOptionsRow.selectedId = filterId;
                    }

                    onWidthChanged: {
                        if (selected) {
                            filterOptionsRow.updateSelectionIndicator(x, width);
                        }
                    }

                    onXChanged: {
                        if (selected) {
                            filterOptionsRow.updateSelectionIndicator(x, width);
                        }
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
                    x: filter_choice_all.x;
                    width: filter_choice_all.width + filterOptionsRow.spacing;
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
                Layout.topMargin: 0;
                Layout.bottomMargin: 4;

                model: root.searchResults;


                 // The standard size
                property var idealCellHeight: 85
                property var idealCellWidth: 85

                // The actual cell height is always as desired, but the cell width
                // is calculated from the current width of the view and how many cells fit
                cellHeight: idealCellHeight
                cellWidth: {
                    return width / Math.floor(width / idealCellWidth)
                }

                readonly property int cellHSpacing: 28;
                readonly property int cellVSpacing: 28;

                focus: true
                clip: true
                interactive: false

                ScrollBar.vertical: ScrollBar {
                    interactive: true
                    policy: ScrollBar.AlwaysOn
                }
                // ScrollIndicator.vertical: ScrollIndicator { }

                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.NoButton

                    hoverEnabled: false
                    propagateComposedEvents: true
                    onWheel: e => {
                        const scrollBar = searchResultList.ScrollBar.vertical
                        const newPos = scrollBar.position - Math.sign(e.angleDelta.y) * 0.1;

                        scrollBar.position = Math.max(0, Math.min(newPos, 1 - scrollBar.size));
                        e.accepted = true;
                    }
                }

                
                delegate: Component {
                    id: itemDelegate

                    MouseArea {
                        id: brush

                        required property int index
                        required property string displayId
                        required property string name
                        required property string resourceString

                        Rectangle {
                            anchors.centerIn: parent
                            color: "blue"
                        }


                        // anchors.centerIn: parent
                        width: searchResultList.idealCellWidth
                        height: searchResultList.cellHeight
                        // width: searchResultList.idealCellWidth // - 20 (suggestion)
                        // height: searchResultList.cellHeight // - 20 (suggestion)
                        // width: childrenRect.width;
                        // height: childrenRect.height;

                        hoverEnabled: true

                        activeFocusOnTab: true;
                        onActiveFocusChanged: {
                            if (activeFocus) {
                                console.log("Focused ", name);
                            }
                        }

                        onPressed: {
                            Context.C_SearchPopupView.brushSelected(index);
                        }

                        Column {
                            anchors.centerIn: parent
                            width: brush.width - 20;
                            height: brush.height - 20;

                            Rectangle {
                                anchors.horizontalCenter: parent.horizontalCenter;
                                width: 32;
                                height: 32;

                                color: "transparent";

                                border.color: "#f2f2f2";
                                border.width: 1;

                                Image {
                                    smooth: false;
                                    anchors.horizontalCenter: parent.horizontalCenter;
                                    source: brush.resourceString;
                                }
                            }

                            Text {
                                width: parent.width;
                                text: brush.displayId;
                                visible: text != "";
                                horizontalAlignment: Text.AlignHCenter;
                            }
                                
                            Text {
                                id: nameText;
                                width: parent.width;
                                text: brush.name;
                                wrapMode: Text.WordWrap;
                                elide: Text.ElideRight;
                                maximumLineCount: brush.containsMouse ? 3 : 1;

                                horizontalAlignment: Text.AlignHCenter;

                                MouseArea {
                                    id: nameTextMouseArea;
                                    anchors.fill: parent;
                                    hoverEnabled: true;
                                }
                            }
                        }
                    }
                }
            }

        }
    }
}