#include "map_view.h"

#include "const.h"

Viewport::Viewport()
    : width(0),
      height(0),
      zoom(1.0f),
      offsetX(0.0f),
      offsetY(0.0f)
{
}

MapView::MapView()
    : map(std::make_shared<Map>()),
      dragState{},
      selection(*this),
      viewport{}
{
}

void MapView::selectTopItem(Tile &tile)
{
  MapAction action = newAction(MapActionType::Selection);
  action.addChange(Change::selectTopItem(tile));

  history.commit(std::move(action));
}

void MapView::deselectTopItem(Tile &tile)
{
  MapAction action = newAction(MapActionType::Selection);
  action.addChange(Change::deselectTopItem(tile));

  history.commit(std::move(action));
}

void MapView::selectAll(Tile &tile)
{
  tile.selectAll();
  selection.select(tile.position);
}

void MapView::clearSelection()
{
  selection.deselectAll();
}

bool MapView::hasSelectionMoveOrigin() const
{
  return selection.moveOrigin.has_value();
}

bool MapView::isSelectionMoving() const
{
  return selection.moving;
}

void MapView::addItem(const Position pos, uint16_t id)
{
  Item item(id);

  const SpriteInfo &spriteInfo = item.itemType->appearance->getSpriteInfo();
  if (spriteInfo.hasAnimation())
  {
    ecs::EntityId entityId = item.assignNewEntityId();
    g_ecs.addComponent(entityId, ItemAnimationComponent(spriteInfo.getAnimation()));
  }

  Tile &currentTile = map->getOrCreateTile(pos);
  Tile newTile = currentTile.deepCopy();
  newTile.addItem(std::move(item));

  MapAction action(*this, MapActionType::SetTile);
  action.addChange(std::move(newTile));
  history.commit(std::move(action));
}

void MapView::removeItems(const Position position, const std::set<size_t, std::greater<size_t>> &indices)
{
  TileLocation *location = map->getTileLocation(position);

  DEBUG_ASSERT(location->hasTile(), "The location has no tile.");

  MapAction action(*this, MapActionType::RemoveTile);

  Tile newTile = deepCopyTile(position);

  for (const auto index : indices)
  {
    newTile.removeItem(index);
  }

  action.addChange(std::move(newTile));

  history.commit(std::move(action));
}

void MapView::removeSelectedItems(const Tile &tile)
{
  MapAction action(*this, MapActionType::ModifyTile);

  Tile newTile = tile.deepCopy();

  for (size_t i = 0; i < tile.items.size(); ++i)
  {
    if (tile.items.at(i).selected)
    {
      newTile.removeItem(i);
    }
  }

  Item *ground = newTile.getGround();
  if (ground && ground->selected)
  {
    newTile.ground.reset();
  }

  action.addChange(Change::setTile(std::move(newTile)));

  history.commit(std::move(action));
}

Tile *MapView::getTile(const Position pos) const
{
  return map->getTile(pos);
}

void MapView::insertTile(Tile &&tile)
{
  MapAction action(*this, MapActionType::SetTile);

  action.addChange(std::move(tile));

  history.commit(std::move(action));
}

void MapView::removeTile(const Position position)
{
  MapAction action(*this, MapActionType::RemoveTile);

  action.addChange(Change::removeTile(position));

  history.commit(std::move(action));
}

void MapView::updateViewport()
{
  viewport.zoom = 1 / camera.zoomFactor;
  viewport.offsetX = camera.position.x;
  viewport.offsetY = camera.position.y;
}

void MapView::setViewportSize(int width, int height)
{
  viewport.width = width;
  viewport.height = height;
}

void MapView::zoomOut()
{
  camera.zoomOut();
}
void MapView::zoomIn()
{
  camera.zoomIn();
}
void MapView::resetZoom()
{
  camera.resetZoom();
}

float MapView::getZoomFactor() const
{
  return camera.zoomFactor;
}

void MapView::translateCamera(glm::vec2 delta)
{
  camera.translate(delta);
}
void MapView::translateCameraZ(int z)
{
  camera.translateZ(z);
}

void MapView::updateCamera(const ScreenPosition cursorPos)
{
  camera.updateZoom(cursorPos);
}

void MapView::deleteSelectedItems()
{
  if (selection.getPositions().empty())
  {
    return;
  }

  history.startGroup(ActionGroupType::RemoveMapItem);
  for (auto pos : selection.getPositions())
  {
    Tile &tile = *getTile(pos);
    if (tile.allSelected())
    {
      removeTile(tile.getPosition());
    }
    else
    {
      removeSelectedItems(tile);
    }
  }

  // TODO: Save the selected item state
  selection.clear();

  history.endGroup(ActionGroupType::RemoveMapItem);
}

util::Rectangle<int> MapView::getGameBoundingRect() const
{
  WorldPosition worldPos{static_cast<double>(viewport.offsetX), static_cast<double>(viewport.offsetY)};
  MapPosition mapPos = worldPos.mapPos();

  auto [width, height] = ScreenPosition{static_cast<double>(viewport.width), static_cast<double>(viewport.height)}.mapPos(*this);
  util::Rectangle<int> rect;
  rect.x1 = mapPos.x;
  rect.y1 = mapPos.y;
  rect.x2 = mapPos.x + width;
  rect.y2 = mapPos.y + height;

  return rect;
}

void MapView::setDragStart(WorldPosition position)
{
  if (dragState.has_value())
  {
    dragState.value().from = position;
  }
  else
  {
    dragState = {position, position};
  }
}

bool MapView::hasSelection() const
{
  return !selection.empty();
}

bool MapView::isEmpty(Position position) const
{
  return map->isTileEmpty(position);
}

void MapView::setDragEnd(WorldPosition position)
{
  DEBUG_ASSERT(dragState.has_value(), "There is no current dragging operation.");

  dragState.value().to = position;
}

void MapView::finishMoveSelection(const Position moveDestination)
{
  if (selection.moving)
  {
    Position deltaPos = moveDestination - selection.moveOrigin.value();

    for (const Position pos : selection.getPositions())
    {
      Position newPos = pos + deltaPos;
      DEBUG_ASSERT(getTile(pos)->hasSelection(), "The tile at each position of a selection should have a selection.");

      selection.deselect(pos);
      map->moveSelectedItems(pos, newPos);
      selection.select(newPos);
    }
  }
  selection.moving = false;
  selection.moveOrigin.reset();
}

void MapView::endDragging()
{
  auto [fromWorldPos, toWorldPos] = dragState.value();

  Position from = fromWorldPos.toPos(*this);
  Position to = toWorldPos.toPos(*this);

  std::unordered_set<Position, PositionHash> positions;

  for (auto &location : map->getRegion(from, to))
  {
    Tile *tile = location.getTile();
    if (tile && !tile->isEmpty())
    {
      positions.emplace(location.getPosition());
    }
  }

  // Only commit a change if anything was dragged over
  if (!positions.empty())
  {
    history.startGroup(ActionGroupType::Selection);

    MapAction action(*this, MapActionType::Selection);

    action.addChange(Change::selection(positions));

    history.commit(std::move(action));
    history.endGroup(ActionGroupType::Selection);
  }

  dragState.reset();
  // This prevents having the mouse release trigger a deselect of the tile being hovered
  selection.blockDeselect = true;
}

bool MapView::isDragging() const
{
  return dragState.has_value();
}

void MapView::panEvent(PanEvent event)
{
  glm::vec2 delta{};
  switch (event.type)
  {
  case PanType::Horizontal:
    delta.x += event.value;
    break;
  case PanType::Vertical:
    delta.y += event.value;
  }

  translateCamera(delta);
}

/*
>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
>>>>>>>>>>>>>>>Internal API>>>>>>>>>>>>>>>>
>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
*/
std::unique_ptr<Tile> MapView::setTileInternal(Tile &&tile)
{
  TileLocation &location = map->getOrCreateTileLocation(tile.position);
  std::unique_ptr<Tile> oldTilePtr = location.replaceTile(std::move(tile));

  if (tile.hasSelection())
  {
    selection.select(tile.position);
  }
  else
  {
    selection.deselect(tile.position);
  }

  return oldTilePtr;
}

std::unique_ptr<Tile> MapView::removeTileInternal(const Position position)
{
  Tile *oldTile = map->getTile(position);
  removeSelectionInternal(oldTile);

  return map->dropTile(position);
}

void MapView::removeSelectionInternal(Tile *tile)
{
  if (tile && tile->hasSelection())
    selection.deselect(tile->position);
}

MapAction MapView::newAction(MapActionType actionType) const
{
  MapAction action(const_cast<MapView &>(*this), actionType);
  return action;
}