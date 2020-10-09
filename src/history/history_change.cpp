#include "history_change.h"

#include "../map_view.h"

namespace MapHistory
{

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
    std::unique_ptr<Tile> currentTilePtr = mapView.setTileInternal(std::move(tile));
    Tile *currentTile = currentTilePtr.release();

    tile = std::move(*currentTile);
  }

  void SetTile::undo(MapView &mapView)
  {
    tile.initEntities();
    std::unique_ptr<Tile> currentTilePointer = mapView.setTileInternal(std::move(tile));
    Tile *currentTile = currentTilePointer.release();

    tile = std::move(*currentTile);
  }

  RemoveTile::RemoveTile(Position pos) : data(pos) {}

  void RemoveTile::commit(MapView &mapView)
  {
    Position &position = std::get<Position>(data);
    std::unique_ptr<Tile> currentTilePointer = mapView.removeTileInternal(position);
    Tile *currentTile = currentTilePointer.release();

    data = std::move(*currentTile);
  }

  void RemoveTile::undo(MapView &mapView)
  {
    Tile &tile = std::get<Tile>(data);
    Position pos = tile.position();

    tile.initEntities();
    mapView.setTileInternal(std::move(tile));

    data = pos;
  }

  Move::Move(Position from, Position to, bool ground, std::vector<uint16_t> &indices)
      : moveData(Move::Partial(ground, std::move(indices))),
        undoData{Tile(from), Tile(to)} {}

  Move::Move(Position from, Position to)
      : moveData(Move::Entire{}), undoData{Tile(from), Tile(to)} {}

  Move::Partial::Partial(bool ground, std::vector<uint16_t> indices)
      : ground(ground), indices(indices) {}

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
    for (int i = 0; i < tile.itemCount(); ++i)
    {
      if (items.at(i).selected)
        indices.emplace_back(i);
    }

    bool moveGround = tile.hasGround() && tile.ground()->selected;

    return Move(tile.position(), to, moveGround, indices);
  }

  void Move::commit(MapView &mapView)
  {
    Position fromPos = undoData.fromTile.position();
    Position toPos = undoData.toTile.position();

    Tile &from = mapView.getOrCreateTile(fromPos);
    Tile &to = mapView.getOrCreateTile(toPos);

    // VME_LOG_D("Moving from " << fromPosition << " to " << toPosition);
    undoData.fromTile = from.deepCopy();
    undoData.toTile = to.deepCopy();

    std::visit(
        util::overloaded{
            [&mapView, &from, &to](const Entire) {
              if (from.hasGround())
              {
                mapView.map()->moveTile(from.position(), to.position());
              }
              else
              {
                for (size_t i = 0; i < from.itemCount(); ++i)
                  to.addItem(from.dropItem(i));
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

    mapView.updateSelection(fromPos);
    mapView.updateSelection(toPos);
  }

  void Move::undo(MapView &mapView)
  {
    mapView.map()->insertTile(std::move(undoData.toTile));
    mapView.map()->insertTile(std::move(undoData.fromTile));
  }

#if _DEBUG_VME
  MultiMove::MultiMove(size_t initialSize)
      : sources(initialSize),
        destinations(initialSize)
  {
    moves.reserve(initialSize);
    sizeof(std::vector<std::vector<Move>>);
  }
#else
  MultiMove::MultiMove(size_t initialSize)
      : destinations(initialSize)
  {
    moves.reserve(initialSize);
  }
#endif

  /*
    Given 'move' (C -> A), returns true if adding this move will create a cycle
    (A -> B -> C -> A)
  */
  bool MultiMove::createsCycle(const Move &move)
  {
    Position C = move.fromPosition();
    Position A = move.toPosition();

    auto cDest = destinations.find(C);
    if (cDest == destinations.end())
      return false;
    auto B = moves.at(cDest->second).fromPosition();
    auto bDest = destinations.find(C);

    if (bDest == destinations.end())
      return false;
    return moves.at(bDest->second).fromPosition() == A;

    // Position C = move.fromPosition();
    // Position A = move.toPosition();
    // auto srcA = sources.find(A);
    // if (srcA == sources.end())
    //   return false;

    // auto B = srcA->second;
    // auto srcB = sources.find(B);
    // return srcB != sources.end() && srcB->second == C;
  }

  void MultiMove::add(Move &&move)
  {
    Position fromPos = move.fromPosition();
    Position toPos = move.toPosition();

    /* 
      If a move A is added with a source that matches the destination of move B,
      then move A must happen before move B.
    */
    auto overlapping = destinations.find(fromPos);
    if (overlapping != destinations.end())
    {
      DEBUG_ASSERT(!createsCycle(move), "Adding this move will cause a cyclic dependency (A -> B, B -> C, C -> A) causing the loss of one tile. Something is probably wrong.");

      uint32_t index = overlapping->second;
      VME_LOG_D(move.fromPosition() << "->" << move.toPosition() << " overlapped by " << moves.at(index).fromPosition() << "->" << moves.at(index).toPosition());

      // Move the overlapping move to the end
      moves.emplace_back(std::move(moves.at(index)));
      overlapping->second = moves.size() - 1;

      moves.at(index) = std::move(move);
      destinations.emplace(toPos, index);
    }
    else
    {
      moves.emplace_back(fromPos, toPos);
      destinations.emplace(toPos, moves.size() - 1);
    }

#if _DEBUG_VME
    sources.emplace(fromPos);
#endif
  }

  void MultiMove::add2(Move &&move)
  {
    Position fromPos = move.fromPosition();
    Position toPos = move.toPosition();

    /* 
      If a move A is added with a source that matches the destination of move B,
      then move A must happen before move B.
    */
    auto overlapping = destinations.find(fromPos);
    if (overlapping != destinations.end())
    {
      DEBUG_ASSERT(sources.find(toPos) == sources.end(), "Adding this move will cause a cyclic dependency (A -> B, B -> C, C -> A) causing the loss of one tile. Something is probably wrong.");

      uint32_t index = overlapping->second;
      // Move the overlapping move to the end
      moves.emplace_back(std::move(moves.at(index)));
      overlapping->second = moves.size() - 1;

      moves.at(index) = std::move(move);
      destinations.emplace(toPos, index);
    }
    else
    {
      moves.emplace_back(fromPos, toPos);
      destinations.emplace(toPos, moves.size() - 1);
    }

#if _DEBUG_VME
    sources.emplace(fromPos);
#endif
  }

  void MultiMove::commit(MapView &mapView)
  {
    VME_LOG_D("\nCommit:");
    for (auto &move : moves)
    {
      const auto show = [](const Position p) {
        std::ostringstream s;
        s << "(" << p.x << ", " << p.y << ")";
        return s.str();
      };
      VME_LOG_D(show(move.fromPosition()) << " -> " << show(move.toPosition()));
      move.commit(mapView);
    }
  }

  void MultiMove::undo(MapView &mapView)
  {
    for (auto it = moves.rbegin(); it != moves.rend(); ++it)
      it->undo(mapView);
  }

  SelectMultiple::SelectMultiple(std::unordered_set<Position, PositionHash> positions, bool select)
      : positions(positions),
        select(select)
  {
  }

  void SelectMultiple::commit(MapView &mapView)
  {
    if (select)
      mapView.selection.merge(positions);
    else
      mapView.selection.deselect(positions);
  }

  void SelectMultiple::undo(MapView &mapView)
  {
    if (select)
      mapView.selection.deselect(positions);
    else
      mapView.selection.merge(positions);
  }

  Select::Select(Position position,
                 std::vector<uint16_t> indices,
                 bool includesGround)
      : position(position),
        indices(indices),
        includesGround(includesGround) {}

  std::optional<Select> Select::fullTile(Tile &tile)
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

  std::optional<Select> Select::topItem(Tile &tile)
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
    Tile *tile = mapView.getTile(position);
    for (const auto i : indices)
      tile->setItemSelected(i, true);

    if (includesGround)
      tile->setGroundSelected(true);

    mapView.selection.setSelected(position, tile->hasSelection());
  }

  void Select::undo(MapView &mapView)
  {
    Tile *tile = mapView.getTile(position);
    for (const auto i : indices)
      tile->setItemSelected(i, false);

    if (includesGround)
      tile->setGroundSelected(false);

    mapView.selection.setSelected(position, tile->hasSelection());
  }

  Deselect::Deselect(Position position,
                     std::vector<uint16_t> indices,
                     bool includesGround)
      : position(position),
        indices(indices),
        includesGround(includesGround) {}

  std::optional<MapHistory::Deselect> Deselect::fullTile(Tile &tile)
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

  std::optional<Deselect> Deselect::topItem(Tile &tile)
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
    Tile *tile = mapView.getTile(position);
    for (const auto i : indices)
      tile->setItemSelected(i, false);

    if (includesGround)
      tile->setGroundSelected(false);

    mapView.selection.setSelected(position, tile->hasSelection());
  }

  void Deselect::undo(MapView &mapView)
  {
    Tile *tile = mapView.getTile(position);
    for (const auto i : indices)
      tile->setItemSelected(i, true);

    if (includesGround)
      tile->setGroundSelected(true);

    mapView.selection.setSelected(position, tile->hasSelection());
  }

  void Action::markAsCommitted()
  {
    committed = true;
  }

} // namespace MapHistory