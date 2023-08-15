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

    static void moveAllExceptPosition(Tile &source, Tile &target);

    Tile(const Tile &) = delete;
    Tile &operator=(const Tile &) = delete;
    Tile(Tile &&other) noexcept;
    Tile &operator=(Tile &&other) noexcept;

    Tile deepCopy(bool onlySelected = false) const;
    Tile deepCopy(Position newPosition) const;

    friend void swap(Tile &first, Tile &second);

    /**
     * @brief Creates a copy of the tile for history purposes.
     * @details This does NOT create a deep copy!
     */
    Tile copyForHistory() const;

    void merge(const Tile &tile);

    inline bool itemSelected(uint16_t itemIndex) const
    {
        return _items.at(itemIndex)->selected;
    }

    void removeSelectedThings();

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
    TileThing getThing(int offsetFromTop) const;
    Item *getTopItem() const;
    inline Item *ground() const noexcept;

    std::optional<size_t> indexOf(Item *item) const;
    Item *itemAt(size_t index);
    Item *itemAt(size_t index) const;

    Item *addBorder(Item &&item, uint32_t zOrder);
    Item *addItem(uint32_t serverId);
    Item *addItem(Item &&item, int insertionOffset = 0);
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
    void clearItems();
    void clearAll();
    void clearBorders();
    void clearBottomItems();

    const Item *getItem(std::function<bool(const Item &)> predicate) const;

    /**
     * @return The amount of removed items
     * */
    template <typename UnaryPredicate>
    inline uint16_t removeItemsIf(UnaryPredicate &&predicate);

    /**
     * @brief Deselects entire tile
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

    /**
     * @brief Returns the top brush for each brush type
     */
    std::vector<Brush *> getTopBrushes();

    int getTopElevation() const;

    const std::vector<std::shared_ptr<Item>> &items() const noexcept
    {
        return _items;
    }

    const size_t itemCount() const noexcept
    {
        return _items.size();
    }

    /**
     * @brief Returns the amount of entities on the tile (items, creature, spawn, waypoint, etc.).
     */
    size_t getEntityCount();

    inline uint16_t mapFlags() const noexcept;
    inline uint16_t statFlags() const noexcept;
    inline uint32_t flags() const noexcept;

    void setFlags(uint32_t flags);

    void setLocation(TileLocation &location);
    void setCreature(Creature &&creature);
    void setCreature(std::shared_ptr<Creature> &&creature);
    void swapCreature(std::shared_ptr<Creature> &creature);
    std::shared_ptr<Creature> dropCreature();
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

    void deepCopyInto(Tile &tile, bool onlySelected) const;

    static void applyToBase(Tile &base, const Tile &other, bool onlySelected = false);

    const std::vector<std::shared_ptr<Item>>::const_iterator findItem(std::function<bool(const Item &)> predicate) const;

    Item *replaceGround(Item &&ground);
    Item *replaceItem(size_t index, Item &&item);
    Creature *replaceCreature(Creature &&creature);

    /**
     * These are shared pointers because they are shared with the history system.
     */
    std::vector<std::shared_ptr<Item>> _items;
    std::shared_ptr<Item> _ground;
    std::shared_ptr<Creature> _creature;

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
