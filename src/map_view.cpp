#include "map_view.h"

#include "../vendor/rollbear-visit/visit.hpp"
#include "brushes/brush.h"
#include "brushes/raw_brush.h"
#include "const.h"
#include "items.h"
#include "time_point.h"

using namespace MapHistory;

std::unordered_set<MapView *> MapView::instances;

MapView::MapView(std::unique_ptr<UIUtils> uiUtils, EditorAction &action)
    : MapView(std::move(uiUtils), action, std::make_shared<Map>())
{
    instances.emplace(this);
}

MapView::MapView(std::unique_ptr<UIUtils> uiUtils, EditorAction &action, std::shared_ptr<Map> map)
    : editorAction(action),
      history(*this),
      _map(map),
      _selection(*this, *map.get()),
      uiUtils(std::move(uiUtils))
{
    instances.emplace(this);
    camera.onViewportChanged<&MapView::cameraViewportChangedEvent>(this);
}

void MapView::cameraViewportChangedEvent()
{
    viewportChange.fire(camera.viewport());
    requestDraw();
}

MapView::~MapView()
{
    instances.erase(this);
}

//>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>Map mutators>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>

void MapView::undo()
{
    bool changed = history.undo();
    if (changed)
    {
        requestDraw();
        undoRedoPerformed.fire();
    }
}

void MapView::redo()
{
    bool changed = history.redo();
    if (changed)
    {
        requestDraw();
        undoRedoPerformed.fire();
    }
}

void MapView::selectTopItem(const Position pos)
{
    Tile *tile = _map->getTile(pos);
    DEBUG_ASSERT(tile != nullptr, "nullptr tile");
    selectTopItem(*tile);
}

void MapView::selectTopItem(const Tile &tile)
{
    auto selection = Select::topItem(tile);
    if (!selection.has_value())
        return;

    Action action(ActionType::Selection);
    action.addChange(std::move(selection.value()));

    history.commit(std::move(action));
}

void MapView::deselectTopItem(const Tile &tile)
{
    auto deselection = Deselect::topItem(tile);
    if (!deselection.has_value())
        return;

    Action action(ActionType::Selection);
    action.addChange(std::move(deselection.value()));

    history.commit(std::move(action));
}

void MapView::selectTile(const Position &pos)
{
    auto tile = getTile(pos);
    if (tile)
        selectTile(*tile);
}

void MapView::selectTile(const Tile &tile)
{
    auto selection = Select::fullTile(tile);
    if (!selection.has_value())
        return;

    Action action(ActionType::Selection);
    action.addChange(std::move(selection.value()));

    history.commit(std::move(action));
}

void MapView::deselectTile(const Position &pos)
{
    auto tile = getTile(pos);
    if (tile)
        deselectTile(*tile);
}

void MapView::deselectTile(const Tile &tile)
{
    auto deselection = Deselect::fullTile(tile);
    if (!deselection.has_value())
        return;

    Action action(ActionType::Selection);
    action.addChange(std::move(deselection.value()));

    history.commit(std::move(action));
}

void MapView::clearSelection()
{
    if (!_selection.empty())
    {
        history.commit(
            ActionType::Selection,
            SelectMultiple(*this, _selection.allPositions(), false));
    }
}

void MapView::modifyTile(const Position pos, std::function<void(Tile &)> f)
{
    Tile &currentTile = _map->getOrCreateTile(pos);
    Tile newTile = currentTile.deepCopy();
    f(newTile);

    history.commit(ActionType::SetTile, SetTile(std::move(newTile)));
}

void MapView::commitTransaction(TransactionType type, std::function<void()> f)
{
    history.beginTransaction(type);
    f();
    history.endTransaction(type);
}

void MapView::beginTransaction(TransactionType transactionType)
{
    history.beginTransaction(transactionType);
}

void MapView::endTransaction(TransactionType transactionType)
{
    history.endTransaction(transactionType);
}

void MapView::addItem(const Position &pos, Item &&item)
{
    Tile &currentTile = _map->getOrCreateTile(pos);
    Tile newTile = currentTile.deepCopy();
    newTile.addItem(std::move(item));

    Action action(ActionType::SetTile);
    action.addChange(SetTile(std::move(newTile)));

    history.commit(std::move(action));
}

void MapView::addItem(const Position &pos, uint16_t id)
{
    if (!Items::items.validItemType(id) || pos.x < 0 || pos.y < 0)
        return;

    addItem(pos, Item(id));
}

void MapView::removeItems(const Position position, const std::set<size_t, std::greater<size_t>> &indices)
{
    TileLocation *location = _map->getTileLocation(position);

    DEBUG_ASSERT(location->hasTile(), "The location has no tile.");

    Action action(ActionType::RemoveTile);

    Tile newTile = deepCopyTile(position);

    for (const auto index : indices)
    {
        newTile.removeItem(index);
    }

    action.addChange(SetTile(std::move(newTile)));

    history.commit(std::move(action));
}

void MapView::removeSelectedItems(const Tile &tile)
{
    Action action(ActionType::ModifyTile);

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

    action.addChange(SetTile(std::move(newTile)));

    history.commit(std::move(action));
}

void MapView::removeItem(Tile &tile, Item *item)
{
    // TODO Commit this to history

    tile.removeItem([item](const Item &_item) { return item == &_item; });
    _selection.updatePosition(tile.position());
    _selection.update();
}

void MapView::removeItem(Tile &tile, std::function<bool(const Item &)> predicate)
{
    // TODO Commit this to history

    tile.removeItem(predicate);
}

void MapView::removeItems(const Tile &tile, std::function<bool(const Item &)> predicate)
{
    Tile newTile = tile.deepCopy();
    if (newTile.removeItemsIf(predicate) > 0)
    {
        Action action(ActionType::ModifyTile);
        action.addChange(SetTile(std::move(newTile)));

        history.commit(std::move(action));
    }
}

void MapView::insertTile(Tile &&tile)
{
    history.commit(ActionType::SetTile, SetTile(std::move(tile)));
}

void MapView::removeTile(const Position position)
{
    Action action(ActionType::RemoveTile);

    action.addChange(RemoveTile(position));

    history.commit(std::move(action));
}

void MapView::moveItem(const Tile &fromTile, const Position toPosition, Item *item)
{
    Action action(ActionType::Move);
    auto move = Move::item(fromTile, toPosition, item);
    if (!move)
    {
        VME_LOG("Warning: tried to move item from tile at position " << fromTile.position() << " but the tile did not contain the item.");
    }

    action.addChange(std::make_unique<Move>(std::move(move.value())));

    history.commit(std::move(action));
}

void MapView::setItemCount(ItemLocation itemLocation, uint8_t count)
{
    Action action(
        ActionType::ModifyItem,
        MapHistory::ModifyItem(std::move(itemLocation), ItemMutation::SetCount(count)));

    history.commit(std::move(action));
}

void MapView::moveFromMapToContainer(Tile &tile, Item *item, ContainerLocation &containerInfo)
{
    DEBUG_ASSERT(tile.indexOf(item) != -1, "The tile must contain the item");

    auto move = MoveFromMapToContainer(tile, item, containerInfo);
    Action action(ActionType::Move, std::move(move));

    history.commit(std::move(action));
}

void MapView::moveFromContainerToMap(ContainerLocation &moveInfo, Tile &tile)
{
    auto move = MoveFromContainerToMap(moveInfo, tile);
    Action action(ActionType::Move, std::move(move));

    history.commit(std::move(action));
}

void MapView::moveFromContainerToContainer(ContainerLocation &from, ContainerLocation &to)
{
    auto move = MoveFromContainerToContainer(from, to);
    Action action(ActionType::Move, std::move(move));

    history.commit(std::move(action));
}

void MapView::moveSelection(const Position &offset)
{
    history.beginTransaction(TransactionType::MoveItems);
    {
        Action action(ActionType::Selection);

        auto multiMove = std::make_unique<MultiMove>(offset, _selection.size());

        for (const auto fromPos : _selection)
        {
            const Tile &fromTile = *getTile(fromPos);
            Position toPos = fromPos + offset;
            DEBUG_ASSERT(fromTile.hasSelection(), "The tile at each position of a selection should have a selection.");

            if (fromTile.allSelected())
                multiMove->add(Move::entire(fromPos, toPos));
            else
                multiMove->add(Move::selected(fromTile, toPos));
        }

        action.addChange(std::move(multiMove));

        history.commit(std::move(action));
    }
    history.endTransaction(TransactionType::MoveItems);
}

void MapView::deleteSelectedItems()
{
    if (_selection.empty())
    {
        return;
    }

    history.beginTransaction(TransactionType::RemoveMapItem);
    for (const auto &pos : _selection)
    {
        const Tile &tile = *getTile(pos);
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
    _selection.clear();

    history.endTransaction(TransactionType::RemoveMapItem);
    requestDraw();
}

void MapView::selectRegion(const Position &from, const Position &to)
{
    std::vector<Position> positions;

    for (auto &location : _map->getRegion(from, to))
    {
        Tile *tile = location.tile();
        if (tile && !tile->isEmpty())
        {
            positions.emplace_back(location.position());
        }
    }

    // Only commit a change if anything was dragged over
    if (!positions.empty())
    {
        history.beginTransaction(TransactionType::Selection);

        Action action(ActionType::Selection);

        action.addChange(SelectMultiple(*this, std::move(positions)));

        history.commit(std::move(action));
        history.endTransaction(TransactionType::Selection);
    }
}

void MapView::removeItemsInRegion(const Position &from, const Position &to, std::function<bool(const Item &)> predicate)
{
    history.beginTransaction(TransactionType::RemoveMapItem);
    for (auto &tileLocation : this->_map->getRegion(from, to))
    {
        if (tileLocation.hasTile())
            removeItems(*tileLocation.tile(), predicate);
    }

    history.endTransaction(TransactionType::RemoveMapItem);
}

void MapView::fillRegion(const Position &from, const Position &to, uint32_t serverId)
{
    if (!Items::items.validItemType(serverId))
        return;

    history.beginTransaction(TransactionType::AddMapItem);

    Action action(ActionType::SetTile);
    action.reserve(Position::tilesInRegion(from, to));

    for (const auto &pos : MapArea(from, to))
    {
        auto location = _map->getTileLocation(pos);

        if (location && location->hasTile())
        {
            Tile newTile(location->tile()->deepCopy());
            newTile.addItem(Item(serverId));

            action.changes.emplace_back<SetTile>(std::move(newTile));
        }
        else
        {
            Tile newTile(pos);
            newTile.addItem(Item(serverId));

            action.changes.emplace_back<SetTile>(std::move(newTile));
        }
    }

    /*
    shrinkToFit appears to reallocate. All tiles are used when filling,
    so there is no advantage to shrinking here.
  */
    // action.shrinkToFit();

    history.commit(std::move(action));

    history.endTransaction(TransactionType::AddMapItem);
}

Item MapView::dropItem(Tile *tile, Item *item)
{
    // TODO Add redo/undo for this action

    Item droppedItem(_map->dropItem(tile, item));

    _selection.updatePosition(tile->position());

    _selection.update();

    return droppedItem;
}

//>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>End of Map mutators>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>

void MapView::setViewportSize(int width, int height)
{
    camera.setSize(width, height);
}

void MapView::setDragStart(WorldPosition position)
{
    if (dragRegion.has_value())
    {
        dragRegion.value().setFrom(position);
    }
    else
    {
        dragRegion = Region2D(position, position);
    }
}

//>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>
//>>>>>>Accessors>>>>>>
//>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>

util::Rectangle<int> MapView::getGameBoundingRect() const
{
    Position position = camera.position();

    const Camera::Viewport &viewport = camera.viewport();
    util::Rectangle<int> rect;
    rect.x1 = position.x;
    rect.y1 = position.y;
    // Add 10 to not miss large sprites (64 in width or height) when zoomed in
    rect.x2 = position.x + viewport.gameWidth() + 10;
    rect.y2 = position.y + viewport.gameHeight() + 10;

    return rect;
}

std::optional<std::pair<WorldPosition, WorldPosition>> MapView::getDragPoints() const
{
    if (!dragRegion.has_value())
        return {};

    std::pair<WorldPosition, WorldPosition> result;
    result.first = dragRegion.value().from();
    result.second = dragRegion.value().to();
    return result;
}

MapRegion MapView::mapRegion() const
{
    Position from(camera.position());
    from.z = from.z <= GROUND_FLOOR ? GROUND_FLOOR : MAP_LAYERS - 1;

    const Camera::Viewport &viewport = camera.viewport();
    Position to(from.x + viewport.gameWidth(), from.y + viewport.gameHeight(), camera.z());

    return _map->getRegion(from, to);
}

Tile *MapView::getTile(const Position pos) const
{
    return _map->getTile(pos);
}

Tile *MapView::getTile(const Position pos)
{
    return const_cast<Tile *>(const_cast<const MapView *>(this)->getTile(pos));
}

Tile &MapView::getOrCreateTile(const Position pos)
{
    return _map->getOrCreateTile(pos);
}

bool MapView::draggingWithSubtract() const
{
    if (!isDragging())
        return false;
    auto brush = editorAction.as<MouseAction::MapBrush>();
    auto modifiers = uiUtils->modifiers();

    return brush && (modifiers & VME::ModifierKeys::Shift) && (modifiers & VME::ModifierKeys::Ctrl);
}

bool MapView::hasSelection() const
{
    return !_selection.empty();
}

bool MapView::singleTileSelected() const
{
    return _selection.size() == 1;
}

const Tile *MapView::singleSelectedTile() const
{
    if (!singleTileSelected())
        return nullptr;

    auto pos = _selection.onlyPosition().value();
    return getTile(pos);
}

Tile *MapView::singleSelectedTile()
{
    return const_cast<Tile *>(const_cast<const MapView *>(this)->singleSelectedTile());
}

bool MapView::singleItemSelected() const
{
    if (!singleTileSelected())
        return nullptr;

    const Position pos = _selection.onlyPosition().value();
    const Tile *tile = getTile(pos);
    DEBUG_ASSERT(tile != nullptr, "A tile that has a selection should never be nullptr.");

    return tile->selectionCount() == 1;
}

const Item *MapView::singleSelectedItem() const
{
    if (!singleTileSelected())
        return nullptr;

    const Position pos = _selection.onlyPosition().value();
    const Tile *tile = getTile(pos);
    DEBUG_ASSERT(tile != nullptr, "A tile that has a selection should never be nullptr.");

    if (tile->selectionCount() != 1)
        return nullptr;

    auto item = tile->firstSelectedItem();
    DEBUG_ASSERT(item != nullptr, "It should be impossible for the selected item to be nullptr.");

    return item;
}

Item *MapView::singleSelectedItem()
{
    const Item *item = const_cast<const MapView *>(this)->singleSelectedItem();
    return const_cast<Item *>(item);
}

bool MapView::isEmpty(const Position position) const
{
    return _map->isTileEmpty(position);
}

bool MapView::isDragging() const
{
    return dragRegion.has_value();
}

bool MapView::inDragRegion(Position pos) const
{
    if (!dragRegion)
        return false;

    WorldPosition topLeft = pos.worldPos();
    WorldPosition bottomRight(topLeft.x + MapTileSize, topLeft.y + MapTileSize);

    return dragRegion.value().collides(topLeft, bottomRight);
}

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>End of Accessors>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>

void MapView::setDragEnd(WorldPosition position)
{
    DEBUG_ASSERT(dragRegion.has_value(), "There is no current dragging operation.");

    dragRegion.value().setTo(position);
}

void MapView::endDragging(VME::ModifierKeys modifiers)
{
    auto [fromWorldPos, toWorldPos] = dragRegion.value();

    Position from = fromWorldPos.toPos(*this);
    Position to = toWorldPos.toPos(*this);

    dragRegion.reset();

    std::visit(
        util::overloaded{
            [this, from, to](MouseAction::Select &select) {
                if (select.area)
                {
                    selectRegion(from, to);

                    // This prevents having the mouse release trigger a deselect of the tile being hovered
                    _selection.blockDeselect = true;

                    select.area = false;
                }
            },

            [this, modifiers, from, to](MouseAction::MapBrush &brush) {
                if (brush.area)
                {
                    if (modifiers & VME::ModifierKeys::Ctrl)
                    {
                        const auto predicate = [brush](const Item &item) {
                            return brush.brush->erasesItem(item.serverId());
                        };
                        removeItemsInRegion(from, to, predicate);
                    }
                    else
                    {
                        if (brush.brush->type() == BrushType::RawItem)
                        {
                            auto rawBrush = static_cast<RawBrush *>(brush.brush);
                            fillRegion(from, to, rawBrush->serverId());
                        }
                        else
                        {
                            // TODO Handle cases other than RawItem
                            NOT_IMPLEMENTED_ABORT();
                        }
                    }

                    brush.area = false;
                }
            },

            [](const auto &arg) {}},
        editorAction.action());

    requestDraw();
}

void MapView::setPanOffset(MouseAction::Pan &action, const ScreenPosition &offset)
{
    if (!action.active())
        return;

    auto newX = static_cast<WorldPosition::value_type>(std::round(offset.x / camera.zoomFactor()));
    auto newY = static_cast<WorldPosition::value_type>(std::round(offset.y / camera.zoomFactor()));

    auto newPosition = action.cameraOrigin.value() + WorldPosition(-newX, -newY);

    if (newPosition.x < 0)
    {
        action.cameraOrigin.value().x -= static_cast<int>(std::round((newPosition.x)));
        newPosition.x = 0;
    }
    if (newPosition.y < 0)
    {
        action.cameraOrigin.value().y -= static_cast<int>(std::round((newPosition.y)));
        newPosition.y = 0;
    }

    camera.setWorldPosition(newPosition);
}

void MapView::startItemDrag(Tile *tile, Item *item)
{
    auto select = editorAction.as<MouseAction::Select>();

    DEBUG_ASSERT(select != nullptr, "editorAction must be select here.");
    select->reset();

    MouseAction::DragDropItem drag(tile, item);
    drag.updateMoveDelta(mouseGamePos());

    editorAction.set(drag);
    editorAction.lock();

    mapItemDragStart.fire(tile, item);
}

//>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>
//>>>>>>Events>>>>>>
//>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>

void MapView::mousePressEvent(VME::MouseEvent event)
{
    // VME_LOG_D("MapView::mousePressEvent");
    if (event.buttons() & VME::MouseButtons::LeftButton)
    {
        Position pos = event.pos().toPos(*this);

        std::visit(
            util::overloaded{
                [this, pos, event](MouseAction::Select &action) {
                    if (event.modifiers() & VME::ModifierKeys::Alt)
                    {
                        Item *topItem = _map->getTopItem(pos);
                        if (topItem)
                        {
                            Tile *tile = _map->getTile(pos);
                            startItemDrag(tile, topItem);
                        }
                    }
                    else if (event.modifiers() & VME::ModifierKeys::Shift)
                    {
                        action.area = true;
                    }
                    else
                    {
                        const Item *topItem = _map->getTopItem(pos);
                        if (!topItem)
                        {
                            commitTransaction(TransactionType::Selection, [this] { clearSelection(); });
                            return;
                        }

                        if (!topItem->selected)
                        {
                            commitTransaction(TransactionType::Selection, [this, pos] {
                                clearSelection();
                                selectTopItem(pos);
                            });
                        }

                        action.setMoveOrigin(pos);
                        editorAction.lock();
                        // VME_LOG_D("Start move: " << pos);
                        // VME_LOG_D("moveDelta: " << _selection.moveDelta());
                    }
                },

                [this, pos, event](MouseAction::MapBrush &action) {
                    commitTransaction(TransactionType::Selection, [this] { clearSelection(); });

                    editorAction.lock();

                    action.area = event.modifiers() & VME::ModifierKeys::Shift;
                    action.erase = event.modifiers() & VME::ModifierKeys::Ctrl;

                    history.beginTransaction(TransactionType::RawItemAction);

                    if (action.erase)
                    {
                        const Tile *tile = getTile(pos);
                        if (tile)
                        {
                            removeItems(*tile, [action](const Item &item) {
                                return action.brush->erasesItem(item.serverId());
                            });
                        }
                    }
                    else
                    {
                        if (!action.area)
                        {
                            if (action.brush->type() == BrushType::RawItem)
                            {
                                auto rawBrush = static_cast<RawBrush *>(action.brush);
                                addItem(pos, rawBrush->serverId());
                            }
                            else
                            {
                                // TODO
                                NOT_IMPLEMENTED_ABORT();
                            }
                        }
                    }
                },

                [this, event](MouseAction::Pan &pan) {
                    editorAction.lock();

                    pan.mouseOrigin = event.pos();
                    pan.cameraOrigin = camera.worldPosition();
                },

                [](const auto &arg) {}},
            editorAction.action());

        auto worldPos = event.pos().worldPos(*this);
        setDragStart(worldPos);

        _previousMouseGamePos = pos;
    }
    requestDraw();
}

void MapView::mouseMoveEvent(VME::MouseEvent event)
{
    Position pos = event.pos().toPos(*this);

    bool newTile = _previousMouseGamePos != pos;

    if (!isDragging())
    {
        if (newTile)
            requestDraw();
        return;
    }

    if (event.buttons() & VME::MouseButtons::LeftButton)
    {
        rollbear::visit(
            util::overloaded{
                [this, &pos, newTile](MouseAction::Select &select) {
                    if (!newTile)
                        return;

                    if (select.moveOrigin.has_value())
                    {
                        select.updateMoveDelta(_selection, pos);
                    }

                    if (select.isMoving() && this->_selection.size() == 1 && !underMouse())
                    {
                        auto tile = singleSelectedTile();

                        if (tile && tile->selectionCount() == 1)
                        {
                            Item *item = tile->firstSelectedItem();

                            editorAction.unlock();
                            startItemDrag(tile, item);
                        }
                    }
                },

                [this, &pos, newTile](MouseAction::DragDropItem &drag) {
                    if (newTile || prevUnderMouse())
                    {
                        if (drag.moveOrigin.has_value())
                        {
                            drag.updateMoveDelta(pos);
                        }
                    }
                },

                [this, pos, &event, newTile](const MouseAction::MapBrush &action) {
                    if (!newTile || action.area)
                    {
                        return;
                    }

                    if (event.modifiers() & VME::ModifierKeys::Ctrl)
                    {
                        const Tile *tile = getTile(pos);
                        if (tile)
                        {
                            removeItems(*tile, [action](const Item &item) {
                                return action.brush->erasesItem(item.serverId());
                            });
                        }
                    }
                    else
                    {
                        if (action.brush->type() == BrushType::RawItem)
                        {
                            uint32_t rawBrushServerId = static_cast<RawBrush *>(action.brush)->serverId();
                            DEBUG_ASSERT(history.hasCurrentTransactionType(TransactionType::RawItemAction), "Incorrect transaction type.");
                            for (const auto position : Position::bresenHams(this->_previousMouseGamePos, pos))
                                addItem(position, rawBrushServerId);
                        }
                        else
                        {
                            // TODO
                            NOT_IMPLEMENTED_ABORT();
                        }
                    }
                },

                [this, event](MouseAction::Pan &action) {
                    ScreenPosition offset = event.pos() - action.mouseOrigin.value();
                    setPanOffset(action, offset);
                },

                [](const auto &arg) {}},
            editorAction.action());

        setDragEnd(event.pos().worldPos(*this));
    }

    _previousMouseGamePos = pos;
    requestDraw();
}

void MapView::mouseReleaseEvent(VME::MouseEvent event)
{
    Position pos = event.pos().toPos(*this);
    // VME_LOG_D("MapView::mouseReleaseEvent: " << pos);

    if (!(event.buttons() & VME::MouseButtons::LeftButton))
    {
        endCurrentAction(event.modifiers());

        if (dragRegion.has_value())
        {
            endDragging(event.modifiers());
        }
    }

    requestDraw();
}

void MapView::itemDropEvent(const ItemDropEvent &event)
{
}

void MapView::dragEnterEvent()
{
    // VME_LOG_D("MapView::dragEnterEvent");
}

void MapView::dragLeaveEvent()
{
    // VME_LOG_D("MapView::dragLeaveEvent");
}

void MapView::endCurrentAction(VME::ModifierKeys modifiers)
{
    std::visit(
        util::overloaded{
            [this](MouseAction::Pan pan) {
                pan.stop();
                editorAction.unlock();
            },

            [this, modifiers](MouseAction::MapBrush &action) {
                if (history.hasCurrentTransactionType(TransactionType::RawItemAction))
                {
                    history.endTransaction(TransactionType::RawItemAction);
                    editorAction.unlock();
                }
            },

            [this](MouseAction::Select &select) {
                if (select.isMoving())
                {
                    waitForDraw([this, &select] {
                        Position deltaPos = select.moveDelta.value();
                        moveSelection(deltaPos);

                        select.reset();
                        editorAction.unlock();
                    });
                }
                else
                {
                    select.reset();
                    editorAction.unlock();
                }
            },

            [this](MouseAction::DragDropItem &drag) {
                if (this->underMouse() && drag.isMoving())
                {
                    waitForDraw([this, &drag] {
                        if (drag.isMoving())
                        {
                            commitTransaction(TransactionType::MoveItems, [this, &drag] {
                                Position toPosition = drag.tile->position() + drag.moveDelta.value();
                                moveItem(*drag.tile, toPosition, drag.item);
                            });
                        }

                        // drag.reset();
                        editorAction.unlock();
                        editorAction.setPrevious();
                    });
                }
                else
                {
                    editorAction.unlock();
                    editorAction.setPrevious();
                }
            },

            [](const auto &arg) {}},
        editorAction.action());
}

void MapView::waitForDraw(std::function<void()> f)
{
    uiUtils->waitForDraw(f);
}

void MapView::escapeEvent()
{
    std::visit(
        util::overloaded{
            [this](MouseAction::Select &) {
                if (!_selection.empty())
                {
                    commitTransaction(TransactionType::Selection, [this] { clearSelection(); });
                    requestDraw();
                }
            },

            [this](const auto &arg) {
                editorAction.reset();
                requestDraw();
            }},
        editorAction.action());
}

void MapView::setViewOption(ViewOption option, bool value)
{
    if (EnumFlag::isSet(_viewOptions, option) != value)
    {
        EnumFlag::set(_viewOptions, option, value);
        requestDraw();
    }
}

void MapView::toggleViewOption(ViewOption option)
{
    EnumFlag::toggle(_viewOptions, option);
    requestDraw();
}

/*
>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
>>>>>>>>>>>>>>>Camera related>>>>>>>>>>>>>>>>
>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
*/

void MapView::setX(WorldPosition::value_type x)
{
    camera.setX(x);
}

void MapView::setY(WorldPosition::value_type y)
{
    camera.setY(y);
}

void MapView::floorUp()
{
    setFloor(floor() - 1);
}

void MapView::floorDown()
{
    setFloor(floor() + 1);
}

void MapView::setFloor(int floor)
{
    camera.setZ(floor);
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

    requestDraw();
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

void MapView::translateX(WorldPosition::value_type x)
{
    camera.setX(camera.x() + x);
}

void MapView::translateY(WorldPosition::value_type y)
{
    camera.setY(camera.y() + y);
}

void MapView::translateZ(int z)
{
    camera.translateZ(z);
}

void MapView::requestDraw()
{
    drawRequest.fire();
}

void MapView::setUnderMouse(bool underMouse)
{
    _prevUnderMouse = _underMouse;

    _underMouse = underMouse;

    if (_prevUnderMouse != underMouse)
    {
        requestDraw();
    }
}

void MapView::perfTest()
{
    VME_LOG("Testing fillregion..");
    TimePoint start;
    Position from(0, 0, 7);
    // Position to(400, 400, 7);
    Position to(2000, 2000, 7);

    fillRegion(from, to, 4526);

    VME_LOG("fillRegion finished in " << start.elapsedMillis() << " ms.");
    TimePoint start2;
    waitForDraw([start2] {
        VME_LOG("draw finished in " << start2.elapsedMillis() << " ms.");
    });

    requestDraw();
}
