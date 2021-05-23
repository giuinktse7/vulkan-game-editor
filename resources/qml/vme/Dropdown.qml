import QtQuick.Controls 2.15
import QtQuick 2.15
import QtQuick.Layouts 1.15
import "./" as Vme


ComboBox {
    id: root;
    height: 25;
    property int popupMaxHeight: 300
    width: 120;
    implicitWidth: 120;
    textRole: "text"
    

    delegate: ItemDelegate {

        width: root.width
        height: root.height
        contentItem: Text {
            text: model.text
            color: Vme.Constants.labelTextColor
            font: root.font
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
        }
        background: Item {}
        highlighted: root.highlightedIndex === index
    }

    contentItem: Text {
        leftPadding: 0
        rightPadding: root.spacing
        clip: true
        width: 120;

        text: root.displayText
        font: root.font
        color: Vme.Constants.labelTextColor
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    background: Rectangle {
        implicitWidth: 120
        implicitHeight: 25
        border.color: root.pressed || root.focus ? "#2196F3" : "#ccc"
        border.width: root.visualFocus ? 2 : 1
    }

    popup: Popup {
        y: root.height - 1
        width: root.width
        implicitHeight: Math.min(contentItem.implicitHeight, root.popupMaxHeight);
        padding: 1


        contentItem: ListView {
            id: popupListView

            ScrollBar.vertical: ScrollBar {}
            interactive: false

            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.NoButton

                hoverEnabled: false
                propagateComposedEvents: true
                onWheel: e => {
                    const scrollBar = popupListView.ScrollBar.vertical
                    const newPos = scrollBar.position - Math.sign(e.angleDelta.y) * 0.1;

                    scrollBar.position = Math.max(0, Math.min(newPos, 1 - scrollBar.size));
                    e.accepted = true;
                }
            }

            snapMode: ListView.NoSnap

            property int currentY: 0

            clip: true
            implicitHeight: contentHeight
            model: root.popup.visible ? root.delegateModel : null
            currentIndex: root.highlightedIndex
            onCurrentIndexChanged: {
                currentY = currentItem ? currentItem.y : 0;
            }

            highlightFollowsCurrentItem: false
            highlight: Component {
                Rectangle {
                    id: highlightRectangle
                    implicitWidth: 120;
                    implicitHeight: 25;
                    color: "#d2d2d2";
                    y: popupListView.currentY;
                }
            }
            ScrollIndicator.vertical: ScrollIndicator { }
        }
    }
}