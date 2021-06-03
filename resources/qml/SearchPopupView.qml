import QtQuick.Controls 2.15
import QtQuick 2.15
import QtQuick.Layouts
import "./vme" as Vme
import Vme.context 1.0 as Context

Rectangle {
    id: root;
    anchors.fill: parent;

    color: "transparent";

    property var searchResults;

    // ShaderEffectSource {
    //     id: theSource
    //     sourceItem: test_rect
    // }

    // ShaderEffect {
    //     anchors.fill: parent;
    //     fragmentShader: "qrc:/test.frag.qsb"
    // }

    // Rectangle {
    //     anchors.fill: parent;
    //     anchors.margins: 14;
            
    //     color: "#fcfcfc";

    //     ColumnLayout {
    //         anchors.fill: parent;
    //         TextField {
    //             Layout.alignment : Qt.AlignTop
    //             objectName: "search_textfield"

    //             selectByMouse: true;
    //             width: root.width;
    //             height: 80
    //             color: "#222222"
    //             background: Item {}
    //             placeholderText: qsTr("Search for brushes...");

    //             onTextChanged: {
    //                 console.log("Search text: ", text);
    //                 Context.C_SearchPopupView.searchEvent(text);
    //             }

    //         }

    //         TabBar {
    //             Layout.alignment : Qt.AlignTop
    //             id: bar
    //             width: parent.width
    //             TabButton {
    //                 text: qsTr("Raw Brush")
    //                 width: implicitWidth
    //             }
    //             TabButton {
    //                 text: qsTr("Ground Brush")
    //                 width: implicitWidth
    //             }
    //             TabButton {
    //                 text: qsTr("Doodad Brush")
    //                 width: implicitWidth
    //             }
    //         }
            
    //         GridView {
    //             id: searchResultList

    //             Layout.fillHeight: true
    //             Layout.fillWidth: true

    //             model: root.searchResults;

    //             cellWidth: 36
    //             cellHeight: 70

    //             focus: true
    //             clip: true
    //             interactive: false

    //             ScrollBar.vertical: ScrollBar {
    //                 interactive: true
    //             }
    //             // ScrollIndicator.vertical: ScrollIndicator { }

    //             MouseArea {
    //                 anchors.fill: parent
    //                 acceptedButtons: Qt.NoButton

    //                 hoverEnabled: false
    //                 propagateComposedEvents: true
    //                 onWheel: e => {
    //                     const scrollBar = searchResultList.ScrollBar.vertical
    //                     const newPos = scrollBar.position - Math.sign(e.angleDelta.y) * 0.06;

    //                     scrollBar.position = Math.max(0, Math.min(newPos, 1 - scrollBar.size));
    //                     e.accepted = true;
    //                 }
    //             }

                
    //             delegate: Component {
    //                 id: itemDelegate

    //                 ColumnLayout {
    //                     id: brush

    //                     required property int serverId
    //                     required property int index

    //                     width: 36

    //                     Rectangle {
    //                         Layout.preferredWidth: 32
    //                         Layout.preferredHeight: 32
    //                         Layout.alignment: Qt.AlignHCenter

    //                         color: "transparent"

    //                         Image {
    //                         anchors.centerIn: parent
    //                             source: {
    //                                 return "image://itemTypes/" + brush.serverId;
    //                             }
    //                         }
    //                     }

    //                     Text {
    //                         Layout.preferredWidth: 32
    //                         Layout.alignment: Qt.AlignHCenter | Qt.AlignTop
    //                         text: brush.serverId
    //                     }
    //                 }
    //             }
    //         }

    //     }
    // }

    // Row {
    //     Item {
    //         id: test_rect
    //         width: 80;
    //         height: 80;

    //         Rectangle {
    //             anchors.centerIn: parent;
    //             width: 50;
    //             height: 50;
    //             color: "black"
    //         }
    //     }
        

    //         Rectangle {
    //             width: 50;
    //             height: 50;
    //             color: "transparent"
    //             border.color: "green"
    //             border.width: 1

    //             ShaderEffect {
    //                 id: shader
    //                 anchors.fill: parent
    //                 property variant src: theSource

    //                 fragmentShader: "qrc:/shadow.frag.qsb"

    //                 property real angle: 0
    //                 NumberAnimation on angle { loops: Animation.Infinite; from: 0; to: Math.PI * 2; duration: 6000 }
    //                 property variant offset: Qt.point(15.0 * Math.cos(angle), 15.0 * Math.sin(angle))
    //                 property variant delta: Qt.size(offset.x / width, offset.y / height)

    //                 property real darkness: 0.5

    //                 property variant shadow: ShaderEffectSource {
    //                     sourceItem: ShaderEffect {
    //                         width: test_rect.width
    //                         height: test_rect.height
    //                         property variant delta: Qt.size(0.0, 1.0 / height)
    //                         property variant source: ShaderEffectSource {
    //                             sourceItem: ShaderEffect {
    //                                 width: test_rect.width
    //                                 height: test_rect.height
    //                                 property variant delta: Qt.size(1.0 / width, 0.0)
    //                                 property variant source: theSource
    //                                 fragmentShader: "qrc:/blur.frag.qsb"
    //                             }
    //                         }
    //                         fragmentShader: "qrc:/blur.frag.qsb"
    //                     }
    //                 }
    //             }
    //         }

            
    //     Text {
    //         text: shader.angle
    //     }
    // }
}