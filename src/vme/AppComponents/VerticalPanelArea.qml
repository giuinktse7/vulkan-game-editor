import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    // Layout.fillHeight: true;
    // Layout.row: 1;
    // Layout.column: 0;
    // Layout.minimumWidth: 200;
    Layout.preferredWidth: gridLayout.childrenRect.width;

    function addItemToLayout(item)
    {
        item.parent = gridLayout;
        item.Layout.row = gridLayout.children.length - 1;
    }

    DropArea {
        id: dropArea;
        anchors.fill: parent;

        property var hoveredChild: null

        onDropped: (drop) => {
            if (drop) {
                drop.acceptProposedAction();

                const item = drop.source;
                if (item.onDetach !== undefined) {
                    item.onDetach = () => gridLayout.beforeRemoveItem(item);
                }

                const droppedOnChild = gridLayout.childAt(0, drop.y);

                item.parent = gridLayout;
                const row = droppedOnChild ? droppedOnChild.Layout.row : gridLayout.children.length - 1;
                item.Layout.row = gridLayout.children.length - 1;
                gridLayout.move(item.Layout.row, row);

                // console.log("CHILD AT: ", gridLayout.childAt(drop.x, drop.y))
            }

            hoveredChild = null;
        }

        onEntered: (drag) => {
        }

        onExited: (drag) => {
            if (drag) {
                drag.source.caught = false;
            }

            hoveredChild = null;
        }

        onPositionChanged: (drag) => {
            const child = gridLayout.childAt(0, drag.y);
            if (!child) {
                hoveredChild = null;
            } else {
                hoveredChild = child;
            }
        }
    }

    GridLayout {
        id: gridLayout;
        columns: 1;
        rowSpacing: 0;
        anchors.left: parent.left;
        anchors.right: parent.right;

        function beforeRemoveItem(item) {
            const row = rowOf(item);

            for (let i = 0; i < children.length; i++) {
                const child = children[i];
                if (child.Layout.row > row) {
                    child.Layout.row -= 1;
                }
            }
        }

        function rowOf(item) {
            for (let i = 0; i < children.length; i++) {
                const child = children[i];
                if (child == item) {
                    return child.Layout.row;
                }
            }

            return -1;
        }

        onChildrenChanged: (k) => {
            // console.log(`onChildrenChange | k: ${k}, children.length: ${children.length}`)
        }

        function showRows() {
            for (let i = 0; i < children.length; i++) {
                const child = children[i];
                console.log(`${i}: ${child.Layout.row}`)
            }
        }

        function childAtRow(r) {
            for (let i = 0; i < children.length; i++) {
                const child = children[i];

                if (child.Layout.row == r) {
                    return child;
                }
            }

            return null;
        }


        function move(fromRow, toRow) {
            // console.log(`Move ${fromRow} -> ${toRow}`);
            if (fromRow == toRow) {
                return;
            }

            const src = childAtRow(fromRow);

            if (fromRow < toRow) {
                for (let i = 0; i < children.length; i++) {
                    const layout = children[i].Layout;
                    if (layout.row > fromRow && layout.row <= toRow) {
                        layout.row -= 1;
                    }
                }
            }
            else if (fromRow > toRow) {
                for (let i = 0; i < children.length; i++) {
                    const layout = children[i].Layout;
                    if (layout.row >= toRow && layout.row < fromRow) {
                        layout.row += 1;
                    }
                }
            }

            src.Layout.row = toRow;
        }
    }

    // Line showing where dropped item will be inserted
    Rectangle {
        width: parent.width;
        height: 3;

        x: 0;
        y: dropArea.hoveredChild ? dropArea.hoveredChild.y : gridLayout.childrenRect.height;
        color: "#42A5F5";
        opacity: 0.4;

        visible: dropArea.containsDrag && gridLayout.children.length > 0;
    }

    // Full-size indicator that is visible when dropping in the first item
    Rectangle {
        width: parent.width;

        anchors {
            top: parent.top
            bottom:  parent.bottom
        }

        color: "#1976D2";
        visible: dropArea.containsDrag && gridLayout.children.length == 0;
        opacity: 0.4;
    }
}
            