#include "history_change.h"

#include "../map_view.h"

namespace MapHistory
{
    Map *ChangeItem::getMap(MapView &mapView) const noexcept
    {
        return mapView._map.get();
    }

    void ChangeItem::updateSelection(MapView &mapView, const Position &position)
    {
        mapView.selection().updatePosition(position);
    }

    void ChangeItem::swapMapTile(MapView &mapView, std::unique_ptr<Tile> &&tile)
    {
        const Position position = tile->position();
        bool selected = tile->hasSelection();

        TileLocation &location = getMap(mapView)->getOrCreateTileLocation(position);
        auto locationTile = location.tile();
        if (locationTile)
        {
            // locationTile->destroyEntities();
            location.swapTile(std::move(tile));
        }
        else
        {
            location.setTile(std::move(tile));
            tile.reset();
        }

        location.tile()->movedInMap();

        mapView.selection().setSelected(position, selected);
    }

    std::unique_ptr<Tile> ChangeItem::setMapTile(MapView &mapView, Tile &&tile)
    {
        const Position position = tile.position();
        bool selected = tile.hasSelection();

        TileLocation &location = getMap(mapView)->getOrCreateTileLocation(position);
        std::unique_ptr<Tile> oldTilePointer = location.replaceTile(std::move(tile));
        if (oldTilePointer)
        {
            // Destroy the ECS entities of the old tile
            oldTilePointer->destroyEntities();
        }

        // updateSelection(mapView, position);
        mapView.selection().setSelected(position, selected);

        return oldTilePointer;
    }

    std::unique_ptr<Tile> ChangeItem::removeMapTile(MapView &mapView, const Position position)
    {
        Map *map = getMap(mapView);
        Tile *oldTile = map->getTile(position);

        if (oldTile && oldTile->hasSelection())
        {
            mapView.selection().deselect(oldTile->position());
        }

        oldTile->destroyEntities();

        return map->dropTile(position);
    }

    void Change::commit(MapView &mapView)
    {
        std::visit(
            util::overloaded{
                [&mapView](std::unique_ptr<ChangeItem> &change) {
                    DEBUG_ASSERT(!change->committed, "Attempted to commit a change that is already marked as committed.");

                    change->commit(mapView);
                    change->committed = true;
                },
                [](std::monostate &s) {
                    DEBUG_ASSERT(false, "[MapHistory::Change::commit] An empty change should never get here.");
                },
                [&mapView](auto &change) {
                    DEBUG_ASSERT(!change.committed, "Attempted to commit a change that is already marked as committed.");

                    change.commit(mapView);
                    change.committed = true;
                }},
            data);
    }

    void Change::undo(MapView &mapView)
    {
        std::visit(
            util::overloaded{
                [&mapView](std::unique_ptr<ChangeItem> &change) {
                    DEBUG_ASSERT(change->committed, "Attempted to undo a change that is not marked as committed.");

                    change->undo(mapView);
                    change->committed = false;
                },
                [](std::monostate &s) {
                    DEBUG_ASSERT(false, "[MapHistory::Change::undo] An empty change should never get here.");
                },
                [&mapView](auto &change) {
                    DEBUG_ASSERT(change.committed, "Attempted to undo a change that is not marked as committed.");

                    change.undo(mapView);
                    change.committed = false;
                }},
            data);
    }

    SetTile::SetTile(Tile &&tile)
        : data(std::make_unique<Tile>(std::move(tile))) {}

    void SetTile::commit(MapView &mapView)
    {
        auto &tile = std::get<std::unique_ptr<Tile>>(data);
        const Position position = tile->position();
        swapMapTile(mapView, std::move(tile));
        if (!tile)
        {
            data = position;
        }
    }

    void SetTile::undo(MapView &mapView)
    {
        if (std::holds_alternative<Position>(data))
        {
            data = getMap(mapView)->dropTile(std::get<Position>(data));
        }
        else
        {
            auto &tile = std::get<std::unique_ptr<Tile>>(data);
            // if (tile)
            // tile->initEntities();

            swapMapTile(mapView, std::move(tile));
        }
    }

    MoveFromMapToContainer::MoveFromMapToContainer(Tile &tile, Item *item, ContainerLocation &to)
        : fromPosition(tile.position()), data(PreFirstCommitData{item}), to(to) {}

    void MoveFromMapToContainer::commit(MapView &mapView)
    {
        auto container = to.container(mapView);
        Tile *tile = mapView.getTile(fromPosition);
        DEBUG_ASSERT(tile != nullptr, "Tile should never be nullptr here.");

        if (std::holds_alternative<PreFirstCommitData>(data))
        {
            auto i = tile->indexOf(std::get<PreFirstCommitData>(data).item);
            DEBUG_ASSERT(i, "Should never be empty.");

            Data newData;
            newData.tileIndex = static_cast<uint16_t>(i.value());

            data = newData;
        }

        container->insertItemTracked(tile->dropItem(std::get<Data>(data).tileIndex), to.containerIndex());
        updateSelection(mapView, tile->position());
    }

    void MoveFromMapToContainer::undo(MapView &mapView)
    {
        Tile *tile = mapView.getTile(fromPosition);
        Data &moveData = std::get<Data>(data);

        tile->insertItem(to.container(mapView)->dropItemTracked(to.containerIndex()), moveData.tileIndex);
        updateSelection(mapView, tile->position());
    }

    MoveFromContainerToMap::MoveFromContainerToMap(ContainerLocation &from, Tile &tile)
        : from(from), toPosition(tile.position()) {}

    void MoveFromContainerToMap::commit(MapView &mapView)
    {
        auto item = from.container(mapView)->dropItem(from.containerIndex());
        mapView.getTile(toPosition)->addItem(std::move(item));
    }

    void MoveFromContainerToMap::undo(MapView &mapView)
    {
        auto item = mapView.getTile(toPosition)->dropItem(static_cast<size_t>(0));
        from.container(mapView)->insertItem(std::move(item), from.containerIndex());
    }

    MoveFromContainerToContainer::MoveFromContainerToContainer(ContainerLocation &from, ContainerLocation &to)
        : from(from), to(to)
    {
        auto relationship = fromToRelationship();
        switch (relationship)
        {
            case Relationship::None:
                break;
            case Relationship::SameContainer:
                sameContainer = true;
                break;
            case Relationship::FromIsChild:
            {
                size_t updateIndex = to.indices.size() - 1;
                int itemContainerIndex = from.indices.at(updateIndex);
                int divergingContainerIndex = to.indices.back();

                if (divergingContainerIndex <= itemContainerIndex)
                {
                    DEBUG_ASSERT(updateIndex < UINT8_MAX, "updateIndex is too big.");
                    indexUpdate = {static_cast<uint8_t>(updateIndex), 1};
                }

                break;
            }
            case Relationship::FromIsParent:
            {
                int updateIndex = from.indices.size() - 1;
                int divergingContainerIndex = to.indices.at(updateIndex);
                int itemContainerIndex = from.indices.back();

                if (itemContainerIndex < divergingContainerIndex)
                {
                    DEBUG_ASSERT(updateIndex < UINT8_MAX, "updateIndex is too big.");

                    indexUpdate = {static_cast<uint8_t>(updateIndex), (int8_t)-1};
                }
                break;
            }
        }
    }

    void MoveFromContainerToContainer::commit(MapView &mapView)
    {
        if (sameContainer)
        {
            from.container(mapView)->moveItemTracked(from.containerIndex(), to.containerIndex());
        }
        else
        {
            to.container(mapView)->insertItemTracked(
                from.container(mapView)->dropItemTracked(from.containerIndex()),
                to.containerIndex());

            if (indexUpdate)
            {
                auto &update = indexUpdate.value();
                auto &location = from.indices.size() > to.indices.size() ? from : to;
                location.indices.at(update.index) += update.delta;
            }
        }
    }

    void MoveFromContainerToContainer::undo(MapView &mapView)
    {
        if (sameContainer)
        {
            from.container(mapView)->moveItemTracked(to.containerIndex(), from.containerIndex());
        }
        else
        {
            from.container(mapView)->insertItemTracked(
                to.container(mapView)->dropItemTracked(to.containerIndex()),
                from.containerIndex());

            if (indexUpdate)
            {
                auto &update = indexUpdate.value();
                auto &location = from.indices.size() > to.indices.size() ? from : to;
                location.indices.at(update.index) -= update.delta;
            }
        }
    }

    MoveFromContainerToContainer::Relationship MoveFromContainerToContainer::fromToRelationship()
    {
        if (from.position != to.position || from.tileIndex != to.tileIndex)
            return Relationship::None;

        auto fromSize = from.indices.size();
        auto toSize = to.indices.size();

        // Last index will be the index of the item in the final container (of either from or to, or both if it was a move within the same container)
        int end = static_cast<int>(std::min(fromSize, toSize) - 1);
        for (int i = 0; i < end; ++i)
        {
            if (from.indices.at(i) != to.indices.at(i))
                return Relationship::None;
        }

        if (fromSize == toSize)
            return Relationship::SameContainer;
        else if (fromSize > toSize)
            return Relationship::FromIsChild;
        else
            return Relationship::FromIsParent;
    }

    RemoveTile::RemoveTile(Position pos)
        : data(pos) {}

    void RemoveTile::commit(MapView &mapView)
    {
        Position &position = std::get<Position>(data);
        std::unique_ptr<Tile> currentTilePointer = removeMapTile(mapView, position);
        Tile *currentTile = currentTilePointer.release();

        data = std::move(*currentTile);

        // Memory no longer handled by the unique ptr, must delete
        delete currentTile;
    }

    void RemoveTile::undo(MapView &mapView)
    {
        Tile &tile = std::get<Tile>(data);
        Position pos = tile.position();

        tile.initEntities();
        setMapTile(mapView, std::move(tile));

        data = pos;
    }

    Move::Move(Position from, Position to)
        : moveData(Move::Entire{}), undoData{Tile(from), Tile(to)} {}

    Move::Move(Position from, Position to, bool ground, std::vector<uint16_t> &indices)
        : moveData(Move::Partial(ground, std::move(indices))),
          undoData{Tile(from), Tile(to)} {}

    Move::Move(Position from, Position to, bool ground)
        : moveData(Move::Partial(ground, std::vector<uint16_t>{})),
          undoData{Tile(from), Tile(to)} {}

    Move::Partial::Partial(bool ground, std::vector<uint16_t> indices)
        : indices(indices), ground(ground) {}

    Move::UndoData::UndoData(Tile &&fromTile, Tile &&toTile)
        : fromTile(std::move(fromTile)), toTile(std::move(toTile)) {}

    Move Move::entire(Position from, Position to)
    {
        return Move(from, to);
    }

    Move Move::entire(const Tile &tile, Position to)
    {
        return Move(tile.position(), to);
    }

    std::optional<Move> Move::item(const Tile &from, const Position &to, Item *item)
    {
        if (item == from.ground())
        {
            return Move(from.position(), to, true);
        }

        auto index = from.indexOf(item);
        if (!index)
            return std::nullopt;

        std::vector<uint16_t> indices{static_cast<uint16_t>(index.value())};
        return Move(from.position(), to, false, indices);
    }

    Move Move::selected(const Tile &tile, Position to)
    {
        std::vector<uint16_t> indices;
        auto &items = tile.items();
        int count = static_cast<int>(tile.itemCount());

        // The indices need to be in descending order for Move::commit to work.
        for (int i = count - 1; i >= 0; --i)
        {
            if (items.at(i).selected)
                indices.emplace_back(i);
        }

        bool moveGround = tile.hasGround() && tile.ground()->selected;

        return Move(tile.position(), to, moveGround, indices);
    }

    void Move::commit(MapView &mapView)
    {
        Map *map = getMap(mapView);
        Position fromPos = undoData.fromTile.position();
        Position toPos = undoData.toTile.position();
        // VME_LOG_D("Commit move: " << fromPos << " -> " << toPos);

        Tile &from = mapView.getOrCreateTile(fromPos);
        Tile &to = mapView.getOrCreateTile(toPos);

        // VME_LOG_D("Moving from " << fromPos << " to " << toPos);
        undoData.fromTile = from.deepCopy();
        undoData.toTile = to.deepCopy();

        std::visit(
            util::overloaded{
                [map, &from, &to](const Entire) {
                    if (from.hasGround())
                    {
                        map->moveTile(from.position(), to.position());
                    }
                    else
                    {
                        from.moveItemsWithBroadcast(to);
                    }
                },

                [&from, &to](const Partial &partial) {
                    for (const auto i : partial.indices)
                    {
                        DEBUG_ASSERT(i < from.itemCount(), "Index out of bounds.");
                        to.addItem(from.dropItem(i));
                    }

                    if (partial.ground && from.hasGround())
                        to.setGround(from.dropGround());
                }},
            moveData);

        // toPos being the first call improves performance (slightly). If toPos is no longer selected,
        // it is possible that selection size becomes 0. If so, caching the only selected
        // position becomes a much faster operation.
        updateSelection(mapView, toPos);
        updateSelection(mapView, fromPos);
    }

    void Move::undo(MapView &mapView)
    {
        Map *map = getMap(mapView);

        map->insertTile(undoData.toTile.deepCopy());
        map->insertTile(undoData.fromTile.deepCopy());

        updateSelection(mapView, undoData.fromTile.position());
        updateSelection(mapView, undoData.toTile.position());
    }

    MultiMove::MultiMove(Position deltaPos, size_t moveOperations)
        : deltaPos(deltaPos)
    {
        moves.reserve(moveOperations);
    }

    void MultiMove::add(Move &&move)
    {
        moves.emplace_back(std::move(move));
    }

    void MultiMove::commit(MapView &mapView)
    {
        DEBUG_ASSERT(!(deltaPos.x == 0 && deltaPos.y == 0), "No-op move is not allowed.");
        std::function<bool(const Move &lhs, const Move &rhs)> f;
        if (!sorted)
        {
            if (deltaPos.x == 0)
            {
                if (deltaPos.y > 0)
                {
                    f = [](const Move &lhs, const Move &rhs) {
                        const Position p1 = lhs.fromPosition();
                        const Position p2 = rhs.fromPosition();
                        return p1.y > p2.y;
                    };
                }
                else
                {
                    f = [](const Move &lhs, const Move &rhs) {
                        const Position p1 = lhs.fromPosition();
                        const Position p2 = rhs.fromPosition();
                        return p1.y < p2.y;
                    };
                }
            }
            else if (deltaPos.y == 0)
            {
                if (deltaPos.x > 0)
                {
                    f = [](const Move &lhs, const Move &rhs) {
                        const Position p1 = lhs.fromPosition();
                        const Position p2 = rhs.fromPosition();
                        return p1.x > p2.x;
                    };
                }
                else
                {
                    f = [](const Move &lhs, const Move &rhs) {
                        const Position p1 = lhs.fromPosition();
                        const Position p2 = rhs.fromPosition();
                        return p1.x < p2.x;
                    };
                }
            }
            else if (deltaPos.x > 0 && deltaPos.y > 0)
            {
                f = [](const Move &lhs, const Move &rhs) {
                    const Position p1 = lhs.fromPosition();
                    const Position p2 = rhs.fromPosition();
                    return p1.x == p2.x ? p1.y > p2.y : p1.x > p2.x;
                };
            }
            else if (deltaPos.x < 0 && deltaPos.y > 0)
            {
                f = [](const Move &lhs, const Move &rhs) {
                    const Position p1 = lhs.fromPosition();
                    const Position p2 = rhs.fromPosition();
                    return p1.x == p2.x ? p1.y > p2.y : p1.x < p2.x;
                };
            }
            else if (deltaPos.x > 0 && deltaPos.y < 0)
            {
                f = [](const Move &lhs, const Move &rhs) {
                    const Position p1 = lhs.fromPosition();
                    const Position p2 = rhs.fromPosition();
                    return p1.x == p2.x ? p1.y < p2.y : p1.x > p2.x;
                };
            }
            else
            {
                f = [](const Move &lhs, const Move &rhs) {
                    const Position p1 = lhs.fromPosition();
                    const Position p2 = rhs.fromPosition();
                    return p1.x == p2.x ? p1.y < p2.y : p1.x < p2.x;
                };
            }
            std::sort(moves.begin(), moves.end(), f);

            sorted = true;
        }

        for (auto &move : moves)
            move.commit(mapView);

        // VME_LOG_D("Moved " << moves.size() << " tiles.");
    }

    void MultiMove::undo(MapView &mapView)
    {
        for (auto it = moves.rbegin(); it != moves.rend(); ++it)
            it->undo(mapView);
    }

    SelectMultiple::SelectMultiple(const MapView &mapView, std::vector<Position> &&positions, bool select)
        : select(select)
    {
        entries.reserve(positions.size());
        for (auto &position : std::move(positions))
        {
            std::vector<uint16_t> indices = getIndices(mapView, position);
            if (!indices.empty())
            {
                entries.emplace_back<Entry>({std::move(position), std::move(indices)});
            }
        }
    }

    void SelectMultiple::commit(MapView &mapView)
    {
        Map *map = getMap(mapView);
        if (select)
        {
            for (auto &entry : entries)
            {
                map->getTile(entry.position)->selectAll();
                mapView.selection().select(entry.position);
            }
        }
        else
        {
            for (auto &entry : entries)
            {
                map->getTile(entry.position)->deselectAll();
                mapView.selection().deselect(entry.position);
            }
        }
    }

    void SelectMultiple::undo(MapView &mapView)
    {
        Map *map = getMap(mapView);
        if (select)
        {
            for (const auto &entry : entries)
            {
                Tile &tile = *map->getTile(entry.position);

                if (!entry.indices.empty())
                {
                    auto it = entry.indices.begin();

                    bool ground = entry.indices.front() == 0;
                    if (ground)
                    {
                        tile.deselectGround();
                        ++it;
                    }
                    while (it != entry.indices.end())
                    {
                        uint16_t i = *it;
                        tile.deselectItemAtIndex(i - 1);
                        ++it;
                    }

                    updateSelection(mapView, entry.position);
                }
            }
        }
        else
        {
            for (const auto &entry : entries)
            {
                Tile &tile = *map->getTile(entry.position);

                if (!entry.indices.empty())
                {
                    auto it = entry.indices.begin();

                    bool ground = entry.indices.front() == 0;
                    if (ground)
                    {
                        tile.selectGround();
                        ++it;
                    }
                    while (it != entry.indices.end())
                    {
                        uint16_t i = *it;
                        tile.selectItemAtIndex(i - 1);
                        ++it;
                    }

                    updateSelection(mapView, entry.position);
                }
            }
        }
    }

    std::vector<uint16_t> SelectMultiple::getIndices(const MapView &mapView, const Position &position) const
    {
        std::vector<uint16_t> result;

        const Tile &tile = *mapView.getTile(position);
        const std::vector<Item> &items = tile.items();

        if (select)
        {
            if (!(tile.hasGround() && tile.ground()->selected))
            {
                // Index 0 corresponds to ground
                result.emplace_back(0);
            }

            for (int i = 0; i < items.size(); ++i)
            {
                if (!tile.itemSelected(i))
                    result.emplace_back(i + 1);
            }
        }
        else
        {
            if (tile.hasGround() && tile.ground()->selected)
            {
                // Index 0 corresponds to ground
                result.emplace_back(0);
            }

            for (int i = 0; i < items.size(); ++i)
            {
                if (tile.itemSelected(i))
                    result.emplace_back(i + 1);
            }
        }

        return result;
    }

    Select::Select(Position position,
                   std::vector<uint16_t> indices,
                   bool includesGround)
        : position(position),
          indices(indices),
          includesGround(includesGround) {}

    std::optional<Select> Select::fullTile(const Tile &tile)
    {
        if (tile.isEmpty())
            return {};

        bool includesGround = tile.hasGround() && !tile.ground()->selected;

        std::vector<uint16_t> indices;
        for (int i = 0; i < tile.itemCount(); ++i)
        {
            if (!tile.itemSelected(i))
            {
                indices.emplace_back(i);
            }
        }

        return Select(tile.position(), indices, includesGround);
    }

    std::optional<Select> Select::topItem(const Tile &tile)
    {
        if (tile.topItemSelected())
            return {};

        std::vector<uint16_t> indices;

        bool isTopGround = tile.getTopItem() == tile.ground();
        if (!isTopGround)
        {
            indices.emplace_back(static_cast<uint16_t>(tile.itemCount() - 1));
        }

        return Select(tile.position(), indices, isTopGround);
    }

    void Select::commit(MapView &mapView)
    {
        Tile *tile = getMap(mapView)->getTile(position);
        for (const auto i : indices)
            tile->setItemSelected(i, true);

        if (includesGround)
            tile->setGroundSelected(true);

        updateSelection(mapView, position);
    }

    void Select::undo(MapView &mapView)
    {
        Tile *tile = getMap(mapView)->getTile(position);
        for (const auto i : indices)
            tile->setItemSelected(i, false);

        if (includesGround)
            tile->setGroundSelected(false);

        updateSelection(mapView, position);
    }

    Deselect::Deselect(Position position,
                       std::vector<uint16_t> indices,
                       bool includesGround)
        : position(position),
          indices(indices),
          includesGround(includesGround) {}

    std::optional<MapHistory::Deselect> Deselect::fullTile(const Tile &tile)
    {
        if (tile.isEmpty())
            return {};

        bool includesGround = tile.hasGround() && tile.ground()->selected;

        std::vector<uint16_t> indices;
        for (int i = 0; i < tile.itemCount(); ++i)
        {
            if (tile.itemSelected(i))
            {
                indices.emplace_back(i);
            }
        }

        return Deselect(tile.position(), indices, includesGround);
    }

    std::optional<Deselect> Deselect::topItem(const Tile &tile)
    {
        if (!tile.topItemSelected())
            return {};

        std::vector<uint16_t> indices;
        bool isTopGround = tile.getTopItem() == tile.ground();
        if (!isTopGround)
        {
            indices.emplace_back(static_cast<uint16_t>(tile.itemCount() - 1));
        }
        return Deselect(tile.position(), indices, isTopGround);
    }

    void Deselect::commit(MapView &mapView)
    {
        Tile *tile = getMap(mapView)->getTile(position);
        for (const auto i : indices)
            tile->setItemSelected(i, false);

        if (includesGround)
            tile->setGroundSelected(false);

        updateSelection(mapView, position);
    }

    void Deselect::undo(MapView &mapView)
    {
        Tile *tile = getMap(mapView)->getTile(position);
        for (const auto i : indices)
            tile->setItemSelected(i, true);

        if (includesGround)
            tile->setGroundSelected(true);

        updateSelection(mapView, position);
    }

    ModifyItem::ModifyItem(ItemLocation &&location, ItemMutation::Mutation &&mutation)
        : location(std::move(location)), mutation(std::move(mutation)) {}

    ModifyItem::ModifyItem(ItemLocation &&location, const ItemMutation::Mutation &mutation)
        : location(std::move(location)), mutation(mutation) {}

    void ModifyItem::commit(MapView &mapView)
    {
        auto item = location.item(mapView);
        std::visit(
            [item](ItemMutation::BaseMutation &itemMutation) {
                itemMutation.commit(item);
            },
            mutation);
    }

    void ModifyItem::undo(MapView &mapView)
    {
        auto item = location.item(mapView);
        std::visit(
            [item](ItemMutation::BaseMutation &itemMutation) {
                itemMutation.undo(item);
            },
            mutation);
    }

    void Action::markAsCommitted()
    {
        committed = true;
    }

} // namespace MapHistory