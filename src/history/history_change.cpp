#include "history_change.h"

#include "../map_view.h"

namespace MapHistory
{
  /*
    Static methods
  */
  std::optional<Select> Select::fullTile(Tile &tile)
  {
    if (tile.isEmpty())
      return {};

    bool includesGround = tile.hasGround() && !tile.getGround()->selected;

    std::vector<uint16_t> indices;
    for (int i = 0; i < tile.getItemCount(); ++i)
    {
      if (!tile.itemSelected(i))
      {
        indices.emplace_back(i);
      }
    }

    return Select(tile.position(), indices, includesGround);
  }

  std::optional<MapHistory::Deselect> Deselect::fullTile(Tile &tile)
  {
    if (tile.isEmpty())
      return {};

    bool includesGround = tile.hasGround() && tile.getGround()->selected;

    std::vector<uint16_t> indices;
    for (int i = 0; i < tile.getItemCount(); ++i)
    {
      if (tile.itemSelected(i))
      {
        indices.emplace_back(i);
      }
    }

    return Deselect(tile.position(), indices, includesGround);
  }

  std::optional<Select> Select::topItem(Tile &tile)
  {
    if (tile.topItemSelected())
      return {};

    std::vector<uint16_t> indices;

    bool isTopGround = tile.getTopItem() == tile.getGround();
    if (!isTopGround)
    {
      indices.emplace_back(static_cast<uint16_t>(tile.getItemCount() - 1));
    }

    return Select(tile.position(), indices, isTopGround);
  }

  std::optional<Deselect> Deselect::topItem(Tile &tile)
  {
    if (!tile.topItemSelected())
      return {};

    std::vector<uint16_t> indices;
    bool isTopGround = tile.getTopItem() == tile.getGround();
    if (!isTopGround)
    {
      indices.emplace_back(static_cast<uint16_t>(tile.getItemCount() - 1));
    }
    return Deselect(tile.position(), indices, isTopGround);
  }

  SetTile::SetTile(Tile &&tile) : tile(std::move(tile)) {}

  void SetTile::commit(MapView &mapView)
  {
    DEBUG_ASSERT(!committed, "Attempted to commit an action that is already marked as committed.");

    std::unique_ptr<Tile> currentTilePtr = mapView.setTileInternal(std::move(tile));
    Tile *currentTile = currentTilePtr.release();

    // TODO Destroy ECS components for the items of the Tile

    tile = std::move(*currentTile);

    committed = true;
  }

  void SetTile::undo(MapView &mapView)
  {
    DEBUG_ASSERT(committed, "Attempted to undo an action that is not marked as committed.");

    std::unique_ptr<Tile> currentTilePtr = mapView.setTileInternal(std::move(tile));
    Tile *currentTile = currentTilePtr.release();

    // TODO Destroy ECS components for the items of the Tile

    tile = std::move(*currentTile);

    committed = false;
  }

  RemoveTile::RemoveTile(Position pos) : data(pos) {}

  void RemoveTile::commit(MapView &mapView)
  {
    Position &position = std::get<Position>(data);
    std::unique_ptr<Tile> tilePtr = mapView.removeTileInternal(position);
    data = std::move(*tilePtr.release());
  }

  void RemoveTile::undo(MapView &mapView)
  {
    Tile &tile = std::get<Tile>(data);
    Position pos = tile.position();

    mapView.setTileInternal(std::move(tile));

    data = pos;
  }

  SelectMultiple::SelectMultiple(std::unordered_set<Position, PositionHash> positions, bool select)
      : positions(positions),
        select(select) {}

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

  void Change::commit(MapView &mapView)
  {
    std::visit(
        util::overloaded{
            [&mapView](std::unique_ptr<ChangeItem> &change) {
              change->commit(mapView);
              change->committed = true;
            },
            [](std::monostate &s) {
              DEBUG_ASSERT(false, "[MapHistory::Change::commit] An empty action should never get here.");
            },
            [&mapView](auto &change) {
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
              change->undo(mapView);
              change->committed = false;
            },
            [](std::monostate &s) {
              DEBUG_ASSERT(false, "[MapHistory::Change::undo] An empty action should never get here.");
            },
            [&mapView](auto &change) {
              change.undo(mapView);
              change.committed = false;
            }},
        data);
  }
} // namespace MapHistory