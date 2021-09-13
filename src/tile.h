#pragma once

#include <algorithm>
#include <memory>
#include <optional>
#include <variant>

#include "creature.h"
#include "item.h"
#include "tile_location.h"

class MapView;
class TileLocation;
class BorderBrush;
class MountainBrush;
class WallBrush;
class GroundBrush;
enum TileCover;
struct TileBorderBlock;

struct BorderCover
{
    BorderCover(TileCover cover, BorderBrush *brush);

    TileCover cover;
    BorderBrush *brush;
};

using TileThing = std::variant<std::monostate, Item *, Creature *>;

class Tile
{
  public:
    Tile(Position position);

    Tile(TileLocation &location);
    // ~Tile();

    Tile(const Tile &) = delete;
    Tile &operator=(const Tile &) = delete;
    Tile(Tile &&other) noexcept;
    Tile &operator=(Tile &&other) noexcept;

    Tile deepCopy(bool onlySelected = false) const;
    Tile deepCopy(Position newPosition) const;

    Tile copyForHistory() const;

    inline bool itemSelected(uint16_t itemIndex) const
    {
        return _items.at(itemIndex)->selected;
    }

    void movedInMap();

    bool topItemSelected() const;
    bool topThingSelected() const;
    bool allSelected() const;
    inline size_t selectionCount() const noexcept;
    Item *firstSelectedItem();
    const Item *firstSelectedItem() const;

    TileThing firstSelectedThing();

    uint8_t minimapColor() const;

    TileThing getTopThing() const;
    Item *getTopItem() const;
    inline Item *ground() const noexcept;

    std::optional<size_t> indexOf(Item *item) const;
    Item *itemAt(size_t index);

    Item *addBorder(Item &&item, uint32_t zOrder);
    Item *addItem(uint32_t serverId);
    Item *addItem(Item &&item);
    Item *addItem(std::shared_ptr<Item> item);

    void insertItem(std::shared_ptr<Item> item, size_t index);
    void insertItem(Item &&item, size_t index);
    void replaceItemByServerId(uint32_t serverId, uint32_t newServerId);

    void removeItem(size_t index);
    void removeItem(Item *item);
    void removeItem(std::function<bool(const Item &)> predicate);
    std::shared_ptr<Item> dropItem(size_t index);
    std::shared_ptr<Item> dropItem(Item *item);
    void removeGround();
    void removeCreature();
    std::shared_ptr<Item> dropGround();
    void setGround(std::shared_ptr<Item> ground);

    void moveItems(Tile &other);
    void moveItemsWithBroadcast(Tile &other);
    void moveSelected(Tile &other);
    void clearBorders();
    void clearBottomItems();

    const Item *getItem(std::function<bool(const Item &)> predicate) const;

    /**
	 * @return The amount of removed items
	 * */
    template <typename UnaryPredicate>
    inline uint16_t removeItemsIf(UnaryPredicate &&predicate);

    /*
		Deselect entire tile
	*/
    void deselectAll();
    void deselectTopItem();
    void selectTopItem();
    void selectItemAtIndex(size_t index);
    void deselectItemAtIndex(size_t index);
    void setItemSelected(size_t itemIndex, bool selected);
    void selectGround();
    void deselectGround();
    void setGroundSelected(bool selected);
    void selectAll();
    void setCreatureSelected(bool selected);

    bool isEmpty() const;

    bool hasSelection() const;
    bool hasTopItem() const;
    bool containsBorder(const BorderBrush *brush) const;
    bool containsItem(std::function<bool(const Item &)> predicate) const;
    bool hasWall(const WallBrush *brush) const;

    // Returns 0 if the border is not present on the tile
    uint32_t getBorderServerId(const BorderBrush *brush) const;
    TileCover getTileCover(const BorderBrush *brush) const;
    TileCover getTileCover(const MountainBrush *brush) const;

    TileBorderBlock getFullBorderTileCover(TileCover excludeMask) const;

    bool hasItems() const;
    inline bool hasGround() const noexcept;
    bool hasBlockingItem() const noexcept;
    inline bool hasCreature() const noexcept;

    GroundBrush *groundBrush() const;

    int getTopElevation() const;

    const std::vector<std::shared_ptr<Item>> &items() const noexcept
    {
        return _items;
    }

    const size_t itemCount() const noexcept
    {
        return _items.size();
    }

    /*
		Counts all entities (items, creature, spawn, waypoint, etc.).
	*/
    size_t getEntityCount();

    inline uint16_t mapFlags() const noexcept;
    inline uint16_t statFlags() const noexcept;
    inline uint32_t flags() const noexcept;

    void setFlags(uint32_t flags);

    void setLocation(TileLocation &location);
    void setCreature(Creature &&creature);
    void setCreature(std::unique_ptr<Creature> &&creature);
    void swapCreature(std::unique_ptr<Creature> &creature);
    std::unique_ptr<Creature> dropCreature();
    inline Creature *creature() const noexcept;

    inline Position position() const noexcept
    {
        return _position;
    }

    inline Position::value_type x() const noexcept
    {
        return _position.x;
    }

    inline Position::value_type y() const noexcept
    {
        return _position.y;
    }

    inline Position::value_type z() const noexcept
    {
        return _position.z;
    }

  private:
    friend class MapView;

    const std::vector<std::shared_ptr<Item>>::const_iterator findItem(std::function<bool(const Item &)> predicate) const;

    Item *replaceGround(Item &&ground);
    Item *replaceItem(size_t index, Item &&item);

    void deepCopyInto(Tile &tile, bool onlySelected) const;

    std::vector<std::shared_ptr<Item>> _items;
    std::shared_ptr<Item> _ground;
    std::unique_ptr<Creature> _creature;

    Position _position;

    // This structure makes it possible to access all flags, or map/stat flags separately.
    union
    {
        struct
        {
            uint16_t _mapflags;
            uint16_t _statflags;
        };
        uint32_t _flags;
    };

    uint16_t _selectionCount;
};

inline uint16_t Tile::mapFlags() const noexcept
{
    return _mapflags;
}

inline uint16_t Tile::statFlags() const noexcept
{
    return _statflags;
}

inline uint32_t Tile::flags() const noexcept
{
    return _flags;
}

template <typename UnaryPredicate>
inline uint16_t Tile::removeItemsIf(UnaryPredicate &&predicate)
{
    uint16_t removedItems = 0;
    if (_ground && std::forward<UnaryPredicate>(predicate)(*(_ground.get())))
    {
        removeGround();
        ++removedItems;
    }

    if (!_items.empty())
    {
        auto removed = std::remove_if(
            _items.begin(),
            _items.end(),
            [this, &predicate, &removedItems](const std::shared_ptr<Item> &item) {
                bool remove = std::forward<UnaryPredicate>(predicate)(*item);

                if (remove)
                {
                    ++removedItems;
                    if (item->selected)
                        --this->_selectionCount;
                }

                return remove;
            });
        _items.erase(removed, _items.end());
    }

    return removedItems;
}

inline bool Tile::hasGround() const noexcept
{
    return static_cast<bool>(_ground);
}

Item *Tile::ground() const noexcept
{
    return _ground.get();
}

inline size_t Tile::selectionCount() const noexcept
{
    return static_cast<size_t>(_selectionCount);
}

inline bool operator==(const Tile &lhs, const Tile &rhs)
{
    const Item *g1 = lhs.ground();
    const Item *g2 = rhs.ground();
    return lhs.flags() == rhs.flags() && lhs.items() == rhs.items() && (!(g1 || g2) || (g1 && g2 && (*g1 == *g2)));
}

inline bool operator!=(const Tile &lhs, const Tile &rhs)
{
    return !(lhs == rhs);
}

inline Creature *Tile::creature() const noexcept
{
    return _creature ? _creature.get() : nullptr;
}

inline bool Tile::hasCreature() const noexcept
{
    return static_cast<bool>(_creature);
}
