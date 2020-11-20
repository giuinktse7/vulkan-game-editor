#include "catch.hpp"

#include "../src/gui/vulkan_window.h"
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
    using MapItem = ItemDragOperation::MimeData::MapItem;

    MapItem mapItem;
    mapItem.mapView = &mapView;
    mapItem.tile = &tile;
    mapItem.item = item;

    auto byteArray = mapItem.toByteArray();
    MapItem deserializedMapView = MapItem::fromByteArray(byteArray);

    REQUIRE(mapItem == deserializedMapView);
  }
}