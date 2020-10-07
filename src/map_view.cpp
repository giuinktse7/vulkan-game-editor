#include "map_view.h"

#include "const.h"
#include "../vendor/rollbear-visit/visit.hpp"

std::unordered_set<MapView *> MapView::instances;

Viewport::Viewport()
    : width(0),
      height(0),
      zoom(0.25f),
      offset(0L, 0L) {}

MapView::MapView(std::unique_ptr<UIUtils> uiUtils, EditorAction &action)
    : MapView(std::move(uiUtils), action, std::make_shared<Map>())
{
  instances.emplace(this);
}

MapView::MapView(std::unique_ptr<UIUtils> uiUtils, EditorAction &action, std::shared_ptr<Map> map)
    : editorAction(action),
      history(*this),
      selection(*this),
      uiUtils(std::move(uiUtils)),
      _map(map),
      viewport(),
      dragState{}
{
  instances.emplace(this);
}

MapView::~MapView()
{
  instances.erase(this);
}

void MapView::selectTopItem(const Position pos)
{
  Tile *tile = getTile(pos);
  DEBUG_ASSERT(tile != nullptr, "nullptr tile");
  selectTopItem(*tile);
}

void MapView::selectTopItem(Tile &tile)
{
  auto selection = MapHistory::Select::topItem(tile);
  if (!selection.has_value())
    return;

  auto action = newAction(MapHistory::ActionType::Selection);
  action.addChange(std::move(selection.value()));

  history.commit(std::move(action));
}

void MapView::deselectTopItem(Tile &tile)
{
  auto deselection = MapHistory::Deselect::topItem(tile);
  if (!deselection.has_value())
    return;

  auto action = newAction(MapHistory::ActionType::Selection);
  action.addChange(std::move(deselection.value()));

  history.commit(std::move(action));
}

void MapView::selectAll(Tile &tile)
{
  tile.selectAll();
  selection.select(tile.position());
}

void MapView::clearSelection()
{
  selection.deselectAll();
}

void MapView::updateSelection(const Position pos)
{
  Tile *tile = getTile(pos);
  selection.setSelected(pos, tile && tile->hasSelection());
}

bool MapView::hasSelectionMoveOrigin() const
{
  return selection.moveOrigin.has_value();
}

void MapView::addItem(const Position pos, uint16_t id)
{
  if (!Items::items.validItemType(id) || pos.x < 0 || pos.y < 0)
    return;

  Tile &currentTile = _map->getOrCreateTile(pos);
  Tile newTile = currentTile.deepCopy();

  newTile.addItem(Item(id));

  MapHistory::Action action(MapHistory::ActionType::SetTile);
  action.addChange(MapHistory::SetTile(std::move(newTile)));

  history.commit(std::move(action));
}

void MapView::removeItems(const Position position, const std::set<size_t, std::greater<size_t>> &indices)
{
  TileLocation *location = _map->getTileLocation(position);

  DEBUG_ASSERT(location->hasTile(), "The location has no tile.");

  MapHistory::Action action(MapHistory::ActionType::RemoveTile);

  Tile newTile = deepCopyTile(position);

  for (const auto index : indices)
  {
    newTile.removeItem(index);
  }

  action.addChange(MapHistory::SetTile(std::move(newTile)));

  history.commit(std::move(action));
}

void MapView::removeSelectedItems(const Tile &tile)
{
  MapHistory::Action action(MapHistory::ActionType::ModifyTile);

  Tile newTile = tile.deepCopy();

  for (size_t i = 0; i < tile.items().size(); ++i)
  {
    if (tile.items().at(i).selected)
      newTile.removeItem(i);
  }

  Item *ground = newTile.ground();
  if (ground && ground->selected)
  {
    newTile._ground.reset();
  }

  action.addChange(MapHistory::SetTile(std::move(newTile)));

  history.commit(std::move(action));
}

Tile *MapView::getTile(const Position pos) const
{
  return _map->getTile(pos);
}

Tile &MapView::getOrCreateTile(const Position pos)
{
  return _map->getOrCreateTile(pos);
}

void MapView::insertTile(Tile &&tile)
{
  MapHistory::Action action(MapHistory::ActionType::SetTile);

  action.addChange(MapHistory::SetTile(std::move(tile)));

  history.commit(std::move(action));
}

void MapView::removeTile(const Position position)
{
  MapHistory::Action action(MapHistory::ActionType::RemoveTile);

  action.addChange(MapHistory::RemoveTile(position));

  history.commit(std::move(action));
}

void MapView::updateViewport()
{
  const float zoom = 1 / camera.zoomFactor();

  bool changed = viewport.offset != camera.position() || viewport.zoom != zoom;

  if (changed)
  {
    viewport.zoom = zoom;
    viewport.offset = camera.position();
    notifyObservers(MapView::Observer::ChangeType::Viewport);
  }
}

void MapView::setViewportSize(int width, int height)
{
  viewport.width = width;
  viewport.height = height;
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
      removeTile(tile.position());
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
  MapPosition mapPos = viewport.offset.mapPos();

  auto [width, height] = ScreenPosition(viewport.width, viewport.height).mapPos(*this);
  util::Rectangle<int> rect;
  rect.x1 = mapPos.x;
  rect.y1 = mapPos.y;
  // Add one to not miss large sprites (64 in width or height) when zoomed in
  rect.x2 = mapPos.x + width + 10;
  rect.y2 = mapPos.y + height + 10;

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
  return _map->isTileEmpty(position);
}

void MapView::setDragEnd(WorldPosition position)
{
  DEBUG_ASSERT(dragState.has_value(), "There is no current dragging operation.");

  dragState.value().to = position;
}

void MapView::finishMoveSelection(const Position moveDestination)
{
  history.startGroup(ActionGroupType::MoveItems);
  {
    MapHistory::Action action(MapHistory::ActionType::Selection);

    Position deltaPos = moveDestination - selection.moveOrigin.value();
    for (const Position fromPos : selection.getPositions())
    {
      const Tile &fromTile = *getTile(fromPos);
      Position toPos = fromPos + deltaPos;
      DEBUG_ASSERT(fromTile.hasSelection(), "The tile at each position of a selection should have a selection.");

      if (fromTile.allSelected())
      {
        auto move = std::make_unique<MapHistory::Move>(MapHistory::Move::entire(fromPos, toPos));
        action.addChange(std::move(move));
      }
      else
      {
        auto move = std::make_unique<MapHistory::Move>(MapHistory::Move::selected(fromTile, toPos));
        action.addChange(std::move(move));
      }
    }

    history.commit(std::move(action));
  }
  history.endGroup(ActionGroupType::MoveItems);

  selection.moveOrigin.reset();
}

void MapView::endDragging(VME::ModifierKeys modifiers)
{
  auto [fromWorldPos, toWorldPos] = dragState.value();

  Position from = fromWorldPos.toPos(*this);
  Position to = toWorldPos.toPos(*this);

  std::visit(
      util::overloaded{
          [this, from, to](const MouseAction::Select select) {
            if (select.area)
            {
              std::unordered_set<Position, PositionHash> positions;

              for (auto &location : _map->getRegion(from, to))
              {
                Tile *tile = location.tile();
                if (tile && !tile->isEmpty())
                {
                  positions.emplace(location.position());
                }
              }

              // Only commit a change if anything was dragged over
              if (!positions.empty())
              {
                history.startGroup(ActionGroupType::Selection);

                MapHistory::Action action(MapHistory::ActionType::Selection);

                action.addChange(MapHistory::SelectMultiple(positions));

                history.commit(std::move(action));
                history.endGroup(ActionGroupType::Selection);
              }

              // This prevents having the mouse release trigger a deselect of the tile being hovered
              selection.blockDeselect = true;

              MouseAction::Select newAction = select;
              newAction.area = false;
              editorAction.set(newAction);
            }
          },

          [this, modifiers, from, to](const MouseAction::RawItem action) {
            if (action.area)
            {
              if (modifiers & VME::ModifierKeys::Ctrl)
              {
                history.startGroup(ActionGroupType::RemoveMapItem);
                for (auto &tileLocation : this->_map->getRegion(from, to))
                {
                  if (tileLocation.hasTile())
                    removeItems(*tileLocation.tile(), [action](const Item &item) { return item.serverId() == action.serverId; });
                }

                history.endGroup(ActionGroupType::RemoveMapItem);
              }
              else
              {
                history.startGroup(ActionGroupType::AddMapItem);
                for (auto pos : MapArea(from, to))
                {
                  addItem(pos, action.serverId);
                }
                history.endGroup(ActionGroupType::AddMapItem);
              }

              MouseAction::RawItem newAction = action;
              newAction.area = false;
              editorAction.set(newAction);
            }
          },

          [](const auto &arg) {}},
      editorAction.action());

  dragState.reset();
}

bool MapView::isDragging() const
{
  return dragState.has_value();
}

void MapView::panEvent(MapView::PanEvent event)
{
  WorldPosition delta{};
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

void MapView::mousePressEvent(VME::MouseEvent event)
{
  // VME_LOG_D("MapView::mousePressEvent");
  if (event.buttons() & VME::MouseButtons::LeftButton)
  {
    Position pos = mouseGamePos();

    std::visit(
        util::overloaded{
            [this, pos, event](const MouseAction::Select action) {
              if (event.modifiers() & VME::ModifierKeys::Shift)
              {
                MouseAction::Select newAction = action;
                newAction.area = true;
                editorAction.set(newAction);
              }
              else
              {
                const Item *topItem = _map->getTopItem(pos);
                if (!topItem)
                {
                  clearSelection();
                  return;
                }

                if (!topItem->selected)
                {
                  clearSelection();
                  history.startGroup(ActionGroupType::Selection);
                  selectTopItem(pos);
                  history.endGroup(ActionGroupType::Selection);
                }

                selection.moveOrigin = pos;
              }
            },

            [this, pos, event](const MouseAction::RawItem &action) {
              clearSelection();

              bool remove = event.modifiers() & VME::ModifierKeys::Ctrl;

              if (event.modifiers() & VME::ModifierKeys::Shift)
              {
                MouseAction::RawItem newAction = action;
                newAction.area = true;
                editorAction.set(newAction);
              }
              else
              {
                if (remove)
                {
                  Tile *tile = getTile(pos);
                  if (tile)
                  {
                    history.startGroup(ActionGroupType::RemoveMapItem);
                    removeItems(*tile, [action](const Item &item) { return item.serverId() == action.serverId; });
                    history.endGroup(ActionGroupType::RemoveMapItem);
                  }
                }
                else
                {
                  history.startGroup(ActionGroupType::AddMapItem);
                  addItem(pos, action.serverId);
                  history.endGroup(ActionGroupType::AddMapItem);
                }
              }
            },
            [](const auto &arg) {}},
        editorAction.action());

    setDragStart(mouseWorldPos());
  }
}

void MapView::mouseMoveEvent(VME::MouseEvent event)
{
  if (!isDragging())
    return;

  auto dragPositions = getDragPoints().value();
  Position pos = event.pos().toPos(*this);

  if (event.buttons() & VME::MouseButtons::LeftButton)
  {
    rollbear::visit(
        util::overloaded{
            [](const MouseAction::Select) {
              // Empty
            },

            [this, pos, event, dragPositions](const MouseAction::RawItem &action) {
              const auto [from, to] = dragPositions;
              if (event.pos().worldPos(*this) == to || action.area)
                return;

              if (event.modifiers() & VME::ModifierKeys::Ctrl)
              {
                Tile *tile = getTile(pos);
                if (tile)
                {
                  history.startGroup(ActionGroupType::RemoveMapItem);

                  removeItems(*tile, [action](const Item &item) { return item.serverId() == action.serverId; });
                  history.endGroup(ActionGroupType::RemoveMapItem);
                }
              }
              else
              {
                int z = this->getFloor();
                history.startGroup(ActionGroupType::AddMapItem);
                for (const auto position : Position::bresenHams(to.toPos(z), pos))
                  addItem(position, action.serverId);
                history.endGroup(ActionGroupType::AddMapItem);
              }
            },

            [](const auto &arg) {}},
        editorAction.action());
  }

  setDragEnd(event.pos().worldPos(*this));
}

void MapView::mouseReleaseEvent(VME::MouseEvent event)
{
  if (!(event.buttons() & VME::MouseButtons::LeftButton))
  {
    if (dragState.has_value())
    {
      endDragging(event.modifiers());
    }
    if (selection.moving())
    {
      finishMoveSelection(mouseGamePos());
    }

    selection.moveOrigin.reset();
  }
}

/*
>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
>>>>>>>>>>>>>>>Camera related>>>>>>>>>>>>>>>>
>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
*/
void MapView::setCameraPosition(WorldPosition position)
{
  camera.setPosition(position);
}

void MapView::setX(long x)
{
  camera.setX(x);
}

void MapView::setY(long y)
{
  camera.setY(y);
}

void MapView::zoom(int delta)
{
  // TODO Handle the value of the zoom delta, not just its sign
  switch (util::sgn(delta))
  {
  case -1:
    zoomOut();
    break;
  case 1:
    zoomIn();
    break;
  default:
    break;
  }
}

void MapView::zoomOut()
{
  camera.zoomOut(mousePos());
}
void MapView::zoomIn()
{
  camera.zoomIn(mousePos());
}
void MapView::resetZoom()
{
  camera.resetZoom(mousePos());
}

float MapView::getZoomFactor() const noexcept
{
  return camera.zoomFactor();
}

void MapView::translateCamera(WorldPosition delta)
{
  camera.translate(delta);
}

void MapView::translateX(long x)
{
  camera.setX(camera.x() + x);
}

void MapView::translateY(long y)
{
  camera.setY(camera.y() + y);
}

void MapView::translateZ(int z)
{
  camera.translateZ(z);
}

void MapView::escapeEvent()
{
  std::visit(
      util::overloaded{
          [this](const MouseAction::Select) {
            clearSelection();
          },

          [this](const auto &&arg) {
            editorAction.reset();
          }},
      editorAction.action());
}

void MapView::addObserver(MapView::Observer *o)
{
  auto found = std::find(observers.begin(), observers.end(), o);
  if (found == observers.end())
  {
    observers.push_back(o);
    o->target = this;
  }
}

void MapView::removeObserver(MapView::Observer *o)
{
  auto found = std::find(observers.begin(), observers.end(), o);
  if (found != observers.end())
  {
    (*found)->target = nullptr;
    observers.erase(found);
  }
}

void MapView::notifyObservers(MapView::Observer::ChangeType changeType) const
{
  switch (changeType)
  {
  case Observer::ChangeType::Viewport:
    for (auto observer : observers)
    {
      observer->viewportChanged(viewport);
    }
    break;
  }
}

MapView::Observer::Observer(MapView *target)
    : target(target)
{
  if (target != nullptr)
  {
    target->addObserver(this);
  }
}

MapView::Observer::~Observer()
{
  if (target)
  {
    target->removeObserver(this);
  }
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
  selection.setSelected(tile.position(), tile.hasSelection());

  TileLocation &location = _map->getOrCreateTileLocation(tile.position());
  std::unique_ptr<Tile> currentTilePointer = location.replaceTile(std::move(tile));

  // Destroy the ECS entities of the old tile
  currentTilePointer->destroyEntities();

  return currentTilePointer;
}

std::unique_ptr<Tile> MapView::removeTileInternal(const Position position)
{
  Tile *oldTile = _map->getTile(position);
  removeSelectionInternal(oldTile);

  oldTile->destroyEntities();

  return _map->dropTile(position);
}

void MapView::removeSelectionInternal(Tile *tile)
{
  if (tile && tile->hasSelection())
    selection.deselect(tile->position());
}

MapHistory::Action MapView::newAction(MapHistory::ActionType actionType) const
{
  MapHistory::Action action(actionType);
  return action;
}
