#include "catch.hpp"

#include "../src/gui/draggable_item.h"
#include "../src/map_view.h"

TEST_CASE("vulkan_window.h", "[gui]")
{
    std::unique_ptr<UIUtils> utils;
    EditorAction editorAction;
    MapView mapView(std::move(utils), editorAction);

    Tile tile(Position(10, 10, 7));
    tile.addItem(Item(2148));

    auto item = tile.getTopItem();

    SECTION("Serialization and Deserialization of ItemDragOperation::MimeData::MapItem preserves the pointers")
    {
        using MapItem = ItemDrag::MapItem;

        MapItem mapItem;
        mapItem.mapView = &mapView;
        mapItem.tile = &tile;
        mapItem._item = item;

        auto byteArray = mapItem.serialize();
        auto deserializedDraggableItem = ItemDrag::DraggableItem::deserialize(byteArray);
        REQUIRE(deserializedDraggableItem);
        MapItem *deserializedMapView = static_cast<MapItem *>(deserializedDraggableItem.get());

        REQUIRE((*deserializedMapView) == mapItem);
    }
}