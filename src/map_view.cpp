#include "map_view.h"

#include "../vendor/rollbear-visit/visit.hpp"
#include "brushes/brush.h"
#include "brushes/creature_brush.h"
#include "brushes/ground_brush.h"
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
    _camera.onViewportChanged<&MapView::cameraViewportChangedEvent>(this);
}

void MapView::cameraViewportChangedEvent()
{
    viewportChange.fire(_camera.viewport());
    requestDraw();
    requestMinimapDraw();
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
        requestMinimapDraw();
        undoRedoPerformed.fire();
    }
}

void MapView::redo()
{
    bool changed = history.redo();
    if (changed)
    {
        requestDraw();
        requestMinimapDraw();
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
    bool e = _selection.empty();
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
        if (tile.items().at(i)->selected)
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

void MapView::addCreature(const Position &pos, Creature &&creature)
{
    auto tile = getTile(pos);
    if (!tile || (tile->hasCreature() && &tile->creature()->creatureType == &creature.creatureType))
    {
        return;
    }

    history.commit(ActionType::ModifyTile, SetCreature(pos, std::move(creature)));
}

void MapView::insertTile(Tile &&tile)
{
    history.commit(ActionType::SetTile, SetTile(std::move(tile)));
}

void MapView::insertTile(std::unique_ptr<Tile> &&tile)
{
    history.commit(ActionType::SetTile, SetTile(std::move(tile)));
}

void MapView::removeTile(const Position position)
{
    Action action(ActionType::RemoveTile);

    // action.addChange(RemoveTile(position));
    action.addChange(std::make_unique<RemoveTile_v2>(position));

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

void MapView::setItemActionId(Item *item, uint16_t actionId)
{
    Action action(
        ActionType::ModifyItem,
        MapHistory::ModifyItem_v2(item, ItemMutation::SetActionId(actionId)));

    history.commit(std::move(action));
}

void MapView::setSubtype(Item *item, uint8_t subtype)
{
    Action action(
        ActionType::ModifyItem,
        MapHistory::ModifyItem_v2(item, ItemMutation::SetSubType(subtype)));

    history.commit(std::move(action));
}

void MapView::setText(Item *item, const std::string &text)
{
    Action action(
        ActionType::ModifyItem,
        MapHistory::ModifyItem_v2(item, ItemMutation::SetText(text)));

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

        for (const auto fromPos : _selection)
        {
            const Tile &fromTile = *getTile(fromPos);
            Position toPos = fromPos + offset;
            DEBUG_ASSERT(fromTile.hasSelection(), "The tile at each position of a selection should have a selection.");

            action.addChange(std::make_unique<Move_v2>(Move_v2::selected(fromTile, toPos)));
        }

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
    requestMinimapDraw();
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
    history.beginTransaction(TransactionType::AddMapItem);

    Action action(ActionType::SetTile);
    action.reserve(Position::tilesInRegion(from, to));

    for (const auto &pos : MapArea(*_map, from, to))
    {
        auto location = _map->getTileLocation(pos);

        Tile newTile = location && location->hasTile() ? location->tile()->deepCopy() : Tile(pos);
        newTile.addItem(Item(serverId));

        action.changes.emplace_back<SetTile>(std::move(newTile));
    }

    history.commit(std::move(action));
    history.endTransaction(TransactionType::AddMapItem);
}

void MapView::fillRegion(const Position &from, const Position &to, std::function<uint32_t()> itemSupplier)
{
    history.beginTransaction(TransactionType::AddMapItem);

    Action action(ActionType::SetTile);
    action.reserve(Position::tilesInRegion(from, to));

    for (const auto &pos : MapArea(*_map, from, to))
    {
        auto location = _map->getTileLocation(pos);

        Tile newTile = location && location->hasTile() ? location->tile()->deepCopy() : Tile(pos);

        uint32_t serverId = itemSupplier();
        newTile.addItem(Item(serverId));

        action.changes.emplace_back<SetTile>(std::move(newTile));
    }

    history.commit(std::move(action));
    history.endTransaction(TransactionType::AddMapItem);
}

std::shared_ptr<Item> MapView::dropItem(Tile *tile, Item *item)
{
    // TODO Add redo/undo for this action

    std::shared_ptr<Item> droppedItem(_map->dropItem(tile, item));

    _selection.updatePosition(tile->position());

    _selection.update();

    return droppedItem;
}

//>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>End of Map mutators>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>

void MapView::rotateBrush()
{
    std::visit(
        util::overloaded{
            [this](MouseAction::MapBrush &brush) {
                brush.rotateClockwise();
                requestDraw();
            },
            [](const auto &arg) {}},
        editorAction.action());
}

void MapView::setViewportSize(int width, int height)
{
    _camera.setSize(width, height);
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
    Position position = _camera.position();

    const Camera::Viewport &viewport = _camera.viewport();
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
    Position from(_camera.position());
    from.z = from.z <= GROUND_FLOOR ? GROUND_FLOOR : MAP_LAYERS - 1;

    const Camera::Viewport &viewport = _camera.viewport();
    Position to(from.x + viewport.gameWidth(), from.y + viewport.gameHeight(), _camera.z());

    return _map->getRegion(from, to);
}

MapRegion MapView::mapRegion(int8_t floor) const
{
    Position from(_camera.position());

    const Camera::Viewport &viewport = _camera.viewport();
    Position to(from.x + viewport.gameWidth(), from.y + viewport.gameHeight(), _camera.z());

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
        return false;

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
                        switch (brush.brush->type())
                        {
                            case BrushType::Raw:
                            {
                                auto rawBrush = static_cast<RawBrush *>(brush.brush);
                                fillRegion(from, to, rawBrush->serverId());
                                break;
                            }
                            case BrushType::Ground:
                            {
                                auto groundBrush = static_cast<GroundBrush *>(brush.brush);
                                fillRegion(from, to, [groundBrush]() { return groundBrush->nextServerId(); });
                                break;
                            }
                            default:
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
    requestMinimapDraw();
}

void MapView::setPanOffset(MouseAction::Pan &action, const ScreenPosition &offset)
{
    if (!action.active())
        return;

    auto newX = static_cast<WorldPosition::value_type>(std::round(offset.x / _camera.zoomFactor()));
    auto newY = static_cast<WorldPosition::value_type>(std::round(offset.y / _camera.zoomFactor()));

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

    _camera.setWorldPosition(newPosition);
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

void MapView::selectTopThing(const Position &position)
{
    selectTopThing(position, true);
}

void MapView::selectTopThing(const Position &position, bool isNewSelection)
{
    Tile *tile = getTile(position);
    if (!tile)
    {
        commitTransaction(TransactionType::Selection, [this] {
            clearSelection();
        });
        return;
    }

    if (tile->topThingSelected())
    {
        selectedTileThingClicked.fire(this, this->getTile(position), tile->getTopThing());
        return;
    }

    if (tile->hasCreature())
    {
        commitTransaction(TransactionType::Selection, [this, position, isNewSelection] {
            if (isNewSelection)
            {
                clearSelection();
            }

            history.commit(ActionType::Selection, SetSelectionTileSpecial::creature(position, true));
        });
    }
    else
    {
        Item *topItem = tile->getTopItem();

        commitTransaction(TransactionType::Selection, [this, position, topItem, isNewSelection] {
            if (isNewSelection)
            {
                clearSelection();
            }

            if (topItem)
            {
                selectTopItem(position);
            }
        });
    }
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
                    // Pickupable item drag
                    if (event.modifiers() & VME::ModifierKeys::Alt)
                    {
                        Item *topItem = _map->getTopItem(pos);
                        if (topItem && topItem->itemType->hasFlag(AppearanceFlag::Take))
                        {
                            Tile *tile = _map->getTile(pos);
                            startItemDrag(tile, topItem);
                            return;
                        }
                    }

                    if (event.modifiers() & VME::ModifierKeys::Shift)
                    {
                        action.area = true;
                    }
                    else
                    {
                        bool isNewSelection = !(event.modifiers() & VME::ModifierKeys::Ctrl);
                        selectTopThing(pos, isNewSelection);

                        action.setMoveOrigin(pos);
                        editorAction.lock();
                    }
                },

                [this, pos, event](MouseAction::MapBrush &action) {
                    commitTransaction(TransactionType::Selection, [this] { clearSelection(); });

                    editorAction.lock();

                    action.area = event.modifiers() & VME::ModifierKeys::Shift;
                    action.erase = event.modifiers() & VME::ModifierKeys::Ctrl;

                    history.beginTransaction(TransactionType::BrushAction);

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
                            action.brush->apply(*this, pos, action.direction);
                        }
                    }
                },

                [this, event](MouseAction::Pan &pan) {
                    editorAction.lock();

                    pan.mouseOrigin = event.pos();
                    pan.cameraOrigin = _camera.worldPosition();
                },
                [this, event](MouseAction::PasteMapBuffer &paste) {
                    std::vector<Position> positions;

                    history.beginTransaction(TransactionType::AddMapItem);
                    this->clearSelection();

                    for (const auto &location : paste.buffer->getBufferMap().begin())
                    {
                        auto newPosition = this->mouseGamePos() + (location->position() - paste.buffer->topLeft);
                        auto tile = location->tile()->deepCopy(newPosition);
                        this->insertTile(std::move(tile));
                        positions.emplace_back(newPosition);
                    }

                    history.endTransaction(TransactionType::AddMapItem);

                    // this->_selection.select(positions);

                    editorAction.reset();
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

        _previousMouseGamePos = pos;
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

                            if (item)
                            {
                                editorAction.unlock();
                                startItemDrag(tile, item);
                            }

                            // TODO Perhaps other things than items will be draggable in the future, ex. creatures.
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
                        DEBUG_ASSERT(history.hasCurrentTransactionType(TransactionType::BrushAction), "Incorrect transaction type.");

                        for (const auto position : Position::bresenHams(this->_previousMouseGamePos, pos))
                        {
                            // Require non-negative positions
                            if (position.x >= 0 && position.y >= 0)
                            {
                                action.brush->apply(*this, position, action.direction);
                            }
                        }

                        requestMinimapDraw();
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

            [this](MouseAction::MapBrush &action) {
                if (history.hasCurrentTransactionType(TransactionType::BrushAction))
                {
                    history.endTransaction(TransactionType::BrushAction);
                    editorAction.unlock();
                }
            },

            [this](MouseAction::Select &select) {
                if (select.isMoving())
                {
                    waitForDraw([this, &select] {
                        Position deltaPos = select.moveDelta.value();
                        moveSelection(deltaPos);
                        requestDraw();
                        requestMinimapDraw();

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
    _camera.setX(x);
}

void MapView::setY(WorldPosition::value_type y)
{
    _camera.setY(y);
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
    _camera.setZ(floor);
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
    requestMinimapDraw();
}

void MapView::zoomOut()
{
    _camera.zoomOut(mousePos());
}

void MapView::zoomIn()
{
    _camera.zoomIn(mousePos());
}

void MapView::resetZoom()
{
    _camera.resetZoom(mousePos());
}

float MapView::getZoomFactor() const noexcept
{
    return _camera.zoomFactor();
}

void MapView::translateCamera(WorldPosition delta)
{
    _camera.translate(delta);
}

void MapView::translateX(WorldPosition::value_type x)
{
    _camera.setX(_camera.x() + x);
}

void MapView::translateY(WorldPosition::value_type y)
{
    _camera.setY(_camera.y() + y);
}

void MapView::translateZ(int z)
{
    _camera.translateZ(z);
}

void MapView::requestDraw()
{
    drawRequest.fire();
}

void MapView::requestMinimapDraw()
{
    drawMinimapRequest.fire();
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

const Camera &MapView::camera() const noexcept
{
    return _camera;
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
