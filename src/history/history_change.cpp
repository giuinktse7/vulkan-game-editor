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

  std::unique_ptr<Tile> ChangeItem::setMapTile(MapView &mapView, Tile &&tile)
  {
    const Position position = tile.position();

    TileLocation &location = getMap(mapView)->getOrCreateTileLocation(tile.position());
    std::unique_ptr<Tile> oldTilePointer = location.replaceTile(std::move(tile));
    if (oldTilePointer)
    {
      // Destroy the ECS entities of the old tile
      oldTilePointer->destroyEntities();
    }

    updateSelection(mapView, position);

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

  SetTile::SetTile(Tile &&tile) : tile(std::move(tile)) {}

  void SetTile::commit(MapView &mapView)
  {
    std::unique_ptr<Tile> currentTilePtr = setMapTile(mapView, std::move(tile));
    Tile *currentTile = currentTilePtr.release();

    tile = std::move(*currentTile);

    // Memory no longer handled by the unique ptr, must delete
    delete currentTile;
  }

  void SetTile::undo(MapView &mapView)
  {
    tile.initEntities();
    std::unique_ptr<Tile> currentTilePointer = setMapTile(mapView, std::move(tile));
    Tile *currentTile = currentTilePointer.release();

    tile = std::move(*currentTile);

    // Memory no longer handled by the unique ptr, must delete
    delete currentTile;
  }

  RemoveTile::RemoveTile(Position pos) : data(pos) {}

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

  Move::Move(Position from, Position to, bool ground, std::vector<uint16_t> &indices)
      : moveData(Move::Partial(ground, std::move(indices))),
        undoData{Tile(from), Tile(to)} {}

  Move::Move(Position from, Position to)
      : moveData(Move::Entire{}), undoData{Tile(from), Tile(to)} {}

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
                from.moveItems(to);
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

    updateSelection(mapView, fromPos);
    updateSelection(mapView, toPos);
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

  void Action::markAsCommitted()
  {
    committed = true;
  }

} // namespace MapHistory