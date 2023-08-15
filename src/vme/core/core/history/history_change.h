#pragma once

#include <functional>
#include <optional>
#include <variant>

#include "../creature.h"
#include "../item_location.h"
#include "../tile.h"
#include "thing_mutation.h"

class MapView;
class Map;

namespace MapHistory
{
    class History;

    enum class ActionType
    {
        Selection,
        ModifyTile,
        SetTile,
        MergeTile,
        RemoveTile,
        CutTile,
        PasteTile,
        Move,
        ModifyItem,
        ModifyCreature
    };

    class Change;
    class ChangeItem
    {
      public:
        ChangeItem()
            : committed(false) {}
        virtual ~ChangeItem() = default;

        ChangeItem(ChangeItem &&other) = default;
        ChangeItem &operator=(ChangeItem &&other) noexcept
        {
            committed = other.committed;
            return *this;
        }

        Map *getMap(MapView &mapView) const noexcept;
        std::unique_ptr<Tile> setMapTile(MapView &mapView, Tile &&tile);
        void swapMapTile(MapView &mapView, std::unique_ptr<Tile> &&tile);
        void swapMapTile(MapView &mapView, Tile &tile);
        std::unique_ptr<Tile> removeMapTile(MapView &mapView, const Position position);
        void updateSelection(MapView &mapView, const Position &position);

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
        SetTile(std::unique_ptr<Tile> &&tile);

        void commit(MapView &mapView) override;
        void undo(MapView &mapView) override;

      protected:
        std::variant<std::unique_ptr<Tile>, Position> data;
    };

    class MergeTile : public ChangeItem
    {
      public:
        MergeTile(Tile &&tile);
        MergeTile(std::unique_ptr<Tile> &&tile);

        void commit(MapView &mapView) override;
        void undo(MapView &mapView) override;

      protected:
        bool firstCommit;
        std::variant<std::unique_ptr<Tile>, Position> data;
    };

    class ModifyItem_v2 : public ChangeItem
    {
      public:
        ModifyItem_v2(Item *item, ItemMutation::Mutation &&mutation);
        ModifyItem_v2(Item *item, const ItemMutation::Mutation &mutation);

        void commit(MapView &mapView) override;
        void undo(MapView &mapView) override;

      private:
        Item *item;
        ItemMutation::Mutation mutation;
    };

    class SetCreatureSpawnInterval : public ChangeItem
    {
      public:
        SetCreatureSpawnInterval(Creature *creature, int spawnInterval);

        void commit(MapView &mapView) override;
        void undo(MapView &mapView) override;

      private:
        Creature *creature;
        int spawnInterval;
    };

    struct ContainerItemMoveInfo
    {
        Tile *tile;
        Item *item;

        size_t containerIndex;
    };

    class MoveFromContainerToContainer : public ChangeItem
    {
        enum class Relationship
        {
            FromIsParent,
            FromIsChild,
            SameContainer,
            None
        };

      public:
        MoveFromContainerToContainer(ContainerLocation &from, ContainerLocation &to);
        void commit(MapView &mapView) override;
        void undo(MapView &mapView) override;

        ContainerLocation from;
        ContainerLocation to;

      private:
        Relationship fromToRelationship();

        // Used when updating index, if there is a relation between from and to that requires it.
        struct IndexUpdate
        {
            uint8_t index;
            int8_t delta;
        };

        bool sameContainer = false;

        std::optional<IndexUpdate> indexUpdate;
    };

    class MoveFromMapToContainer : public ChangeItem
    {
      public:
        MoveFromMapToContainer(Tile &tile, Item *item, ContainerLocation &to);
        void commit(MapView &mapView) override;
        void undo(MapView &mapView) override;

      private:
        Position fromPosition;
        ContainerLocation to;

        struct PreFirstCommitData
        {
            Item *item;
        };

        struct Data
        {
            uint16_t tileIndex;
        };

        std::variant<PreFirstCommitData, Data> data;
    };

    class MoveFromContainerToMap : public ChangeItem
    {
      public:
        MoveFromContainerToMap(ContainerLocation &from, Tile &tile);
        void commit(MapView &mapView) override;
        void undo(MapView &mapView) override;

      private:
        ContainerLocation from;
        Position toPosition;
    };

    class RemoveTile_v2 : public ChangeItem
    {
      public:
        RemoveTile_v2(Position pos);

        void commit(MapView &mapView) override;
        void undo(MapView &mapView) override;

      private:
        std::variant<std::unique_ptr<Tile>, Position> data;
    };

    class RemoveTile : public ChangeItem
    {
      public:
        RemoveTile(Position pos);

        void commit(MapView &mapView) override;
        void undo(MapView &mapView) override;

      private:
        std::variant<Tile, Position> data;
    };

    class MoveSelection : public ChangeItem
    {
      public:
        MoveSelection(std::vector<Position> positions, Position deltaPos);
        void commit(MapView &mapView) override;
        void undo(MapView &mapView) override;

      private:
        struct MoveData
        {
            Tile fromTile;
            Tile toTile;
        };

        std::unique_ptr<std::vector<Position>> &dataAsPositions()
        {
            return std::get<std::unique_ptr<std::vector<Position>>>(moveData);
        }

        std::unique_ptr<std::vector<MoveData>> &dataAsTiles()
        {
            return std::get<std::unique_ptr<std::vector<MoveData>>>(moveData);
        }

        std::function<bool(const Position &, const Position &)> getPositionComparator();

        Position deltaPos;

        std::variant<std::unique_ptr<std::vector<MoveData>>, std::unique_ptr<std::vector<Position>>> moveData;

        bool firstCommit = true;
    };

    class Move_v2 : public ChangeItem
    {
      public:
        enum MoveFlags : uint8_t
        {
            MoveGround = 1,
            MoveCreature = 1 << 1
        };

        Move_v2(Position from, Position to);
        Move_v2(Position from, Position to, MoveFlags moveFlags);
        Move_v2(Position from, Position to, MoveFlags moveFlags, std::vector<uint16_t> indices);

        static Move_v2 entire(Position from, Position to);
        static Move_v2 entire(const Tile &tile, Position to);

        static Move_v2 selected(const Tile &tile, Position to);

        void commit(MapView &mapView) override;
        void undo(MapView &mapView) override;

        Position fromPos() const
        {
            return fromTile.position();
        }

        Position toPos() const
        {
            return toTile.position();
        }

      private:
        struct PartialMoveData
        {
            std::vector<uint16_t> indices;
            MoveFlags moveFlags;
        };
        std::optional<PartialMoveData> partialMoveData;

        Tile fromTile;
        Tile toTile;

        bool firstCommit = true;
    };

    /*
    NOTE: The 'from' and 'to' positions are stored in the undoData tiles to
    save space
  */
    class Move : public ChangeItem
    {
      public:
        Move(Position from, Position to);
        Move(Position from, Position to, bool ground);
        Move(Position from, Position to, bool ground, std::vector<uint16_t> &indices);

        static Move entire(Position from, Position to);
        static Move entire(const Tile &tile, Position to);

        static Move selected(const Tile &tile, Position to);
        static std::optional<Move> item(const Tile &from, const Position &to, Item *item);

        void commit(MapView &mapView) override;
        void undo(MapView &mapView) override;

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

    class SetCreature : public ChangeItem
    {
      public:
        SetCreature(Position position, std::shared_ptr<Creature> &&creature);
        SetCreature(Position position, Creature &&creature);

        static SetCreature noCreature(Position position);

        void commit(MapView &mapView) override;
        void undo(MapView &mapView) override;

      private:
        Position position;
        std::shared_ptr<Creature> creature;
    };

    class SelectMultiple : public ChangeItem
    {
      public:
        SelectMultiple(const MapView &mapView, std::vector<Position> &&positions, bool select = true);

        void commit(MapView &mapView) override;
        void undo(MapView &mapView) override;

      private:
        struct Entry
        {
            Position position;

            /**
             * To save space, the ground tile is represented by the value 0. Hence
             * item indices are offset by 1. The item at index 0 in a tile is
             * represented by the value 1 here, etc.
             * 0: Ground
             * 1..n: Items
             */
            std::vector<uint16_t> indices;

            bool creature = false;
        };

        std::vector<Entry> entries;
        bool select;

        Entry getEntry(const MapView &mapView, const Tile &tile) const;
    };

    class Select : public ChangeItem
    {
      public:
        Select(Position position,
               std::vector<uint16_t> indices,
               bool includesGround = false);

        static std::optional<MapHistory::Select> fullTile(const Tile &tile);
        static std::optional<MapHistory::Select> topItem(const Tile &tile);

        void commit(MapView &mapView) override;
        void undo(MapView &mapView) override;

      private:
        Position position;
        std::vector<uint16_t> indices;
        bool includesGround = false;
    };

    // Select tile special (currently only creature)
    class SetSelectionTileSpecial : public ChangeItem
    {
      public:
        enum class ThingType
        {
            Creature,
            Spawn
        };

        SetSelectionTileSpecial(Position position, ThingType thingType, bool selected);

        static SetSelectionTileSpecial creature(Position position, bool selected);
        static SetSelectionTileSpecial spawn(Position position, bool selected);

        void commit(MapView &mapView) override;
        void undo(MapView &mapView) override;

      private:
        Position position;
        ThingType thingType;
        bool selected;
    };

    class Deselect : public ChangeItem
    {
      public:
        Deselect(Position position,
                 std::vector<uint16_t> indices,
                 bool includesGround = false);

        static std::optional<MapHistory::Deselect> fullTile(const Tile &tile);
        static std::optional<MapHistory::Deselect> topItem(const Tile &tile);

        void commit(MapView &mapView) override;
        void undo(MapView &mapView) override;

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
        using DataTypes = std::variant<
            std::monostate,
            SetTile,
            MergeTile,
            RemoveTile,
            Select,
            Deselect,
            SelectMultiple,
            SetSelectionTileSpecial,
            MoveFromMapToContainer,
            MoveFromContainerToMap,
            MoveFromContainerToContainer,
            ModifyItem_v2,
            SetCreatureSpawnInterval,
            SetCreature,
            std::unique_ptr<ChangeItem>>;

        Change(DataTypes data = {})
            : data(std::move(data)) {}

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
        Change()
            : data({}) {}
    };

} // namespace MapHistory

VME_ENUM_OPERATORS(MapHistory::Move_v2::MoveFlags)