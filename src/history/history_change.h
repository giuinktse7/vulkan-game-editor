#pragma once

#include <optional>
#include <variant>

#include "../tile.h"

class MapView;

namespace MapHistory
{
  class History;

  enum class ActionType
  {
    Selection,
    ModifyTile,
    SetTile,
    Move,
    RemoveTile,
    CutTile,
    PasteTile
  };

  class Change;
  class ChangeItem
  {
  public:
    ChangeItem() : committed(false) {}
    virtual ~ChangeItem() {}

    ChangeItem(ChangeItem &&other) = default;
    ChangeItem &operator=(const ChangeItem &&other) noexcept
    {
      committed = other.committed;
      return *this;
    }

    virtual void commit(MapView &mapView) = 0;
    virtual void redo(MapView &mapView)
    {
      commit(mapView);
    }
    virtual void undo(MapView &mapView) = 0;

  protected:
    friend class MapHistory::Change;
    bool committed;
  };

  class SetTile : public ChangeItem
  {
  public:
    SetTile(Tile &&tile);

    virtual void commit(MapView &mapView) override;
    virtual void undo(MapView &mapView) override;

  private:
    Tile tile;
  };

  class RemoveTile : public ChangeItem
  {
  public:
    RemoveTile(Position pos);

    virtual void commit(MapView &mapView) override;
    virtual void undo(MapView &mapView) override;

  private:
    std::variant<Tile, Position> data;
  };

  /* 
    NOTE: The 'from' and 'to' positions are stored in the undoData tiles to
    save space
  */
  class Move : public ChangeItem
  {
  public:
    Move(Position from, Position to, bool ground, std::vector<uint16_t> &indices);
    Move(Position from, Position to);

    static Move entire(Position from, Position to);
    static Move entire(const Tile &tile, Position to);

    static Move selected(const Tile &tile, Position to);

    virtual void commit(MapView &mapView) override;
    virtual void undo(MapView &mapView) override;

    inline Position fromPosition() const noexcept
    {
      return undoData.fromTile.position();
    }
    inline Position toPosition() const noexcept
    {
      return undoData.toTile.position();
    }

  private:
    struct Entire
    {
    };

    struct Partial
    {
      Partial(bool ground, std::vector<uint16_t> indices);
      std::vector<uint16_t> indices;
      bool ground;
    };

    std::variant<Entire, Partial> moveData;

    struct UndoData
    {
      UndoData(Tile &&fromTile, Tile &&toTile);
      Tile fromTile;
      Tile toTile;
    };

    UndoData undoData;
  };

  class MultiMove : public ChangeItem
  {
  public:
    MultiMove(Position deltaPos, size_t moveOperations);
    virtual void commit(MapView &mapView) override;
    virtual void undo(MapView &mapView) override;

    void add(Move &&move);

  private:
    Position deltaPos;

    std::vector<Move> moves;
    bool sorted = false;
  };

  class SelectMultiple : public ChangeItem
  {
  public:
    SelectMultiple(std::vector<Position> positions, bool select = true);

    virtual void commit(MapView &mapView) override;
    virtual void undo(MapView &mapView) override;

  private:
    std::vector<Position> positions;
    bool select;
  };

  class Select : public ChangeItem
  {
  public:
    Select(Position position,
           std::vector<uint16_t> indices,
           bool includesGround = false);

    static std::optional<MapHistory::Select> fullTile(Tile &tile);
    static std::optional<MapHistory::Select> topItem(Tile &tile);

    virtual void commit(MapView &mapView) override;
    virtual void undo(MapView &mapView) override;

  private:
    Position position;
    std::vector<uint16_t> indices;
    bool includesGround = false;
  };

  class Deselect : public ChangeItem
  {
  public:
    Deselect(Position position,
             std::vector<uint16_t> indices,
             bool includesGround = false);

    static std::optional<MapHistory::Deselect> fullTile(Tile &tile);
    static std::optional<MapHistory::Deselect> topItem(Tile &tile);

    virtual void commit(MapView &mapView) override;
    virtual void undo(MapView &mapView) override;

  private:
    Position position;
    std::vector<uint16_t> indices;
    bool includesGround = false;
  };

  /**
      NOTE: Each change variant requires space equal to the space required
            for the largest variant type. Therefore, if a data type is
            larger than ~100 bytes, it is stored as a pointer in the form
            std::unique_ptr<ChangeItem> instead.
    */
  class Change
  {
  public:
    using DataTypes = std::variant<std::monostate, SetTile, RemoveTile, Select, Deselect, SelectMultiple, std::unique_ptr<ChangeItem>>;

    Change(DataTypes data = {}) : data(std::move(data)) {}

    Change(const Change &other) = delete;
    Change &operator=(const Change &other) = delete;

    Change(Change &&other) noexcept
        : data(std::move(other.data)) {}

    Change &operator=(Change &&other) noexcept
    {
      data = std::move(other.data);
      return *this;
    }

    void commit(MapView &mapView);
    void undo(MapView &mapView);

    DataTypes data;

  private:
    Change() : data({}) {}
  };

} // namespace MapHistory