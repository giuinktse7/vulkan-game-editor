#pragma once

#include <array>
#include <limits>
#include <memory>
#include <optional>
#include <queue>
#include <sstream>
#include <type_traits>
#include <vector>

#include "debug.h"
#include "position.h"
#include "time_point.h"
#include "util.h"

// HeapNode macro
#define DECLARE_NODE(name, amount)                                                \
  class name : public BaseNode<amount>                                            \
  {                                                                               \
  public:                                                                         \
    name(const Position midPoint, const SplitDelta splitDelta, HeapNode *parent); \
                                                                                  \
    HeapNode *getOrCreateChild(const Position pos) override;                      \
    std::string show() const override;                                            \
                                                                                  \
  private:                                                                        \
    Position midPoint;                                                            \
    SplitDelta splitDelta;                                                        \
                                                                                  \
    uint16_t getIndex(const Position &pos) const override;                        \
  }

enum SplitDimension
{
  X = 1 << 2,
  Y = 1 << 1,
  Z = 1 << 0,
};
VME_ENUM_OPERATORS(SplitDimension)

namespace vme
{
  namespace octree
  {
    class Tree;
    class Leaf;

    template <typename T>
    constexpr T power(T num, uint32_t pow)
    {
      T result = 1;
      for (; pow >= 1; --pow)
        result *= num;

      return result;
    }

    struct Cube
    {
      uint32_t width;
      uint32_t height;
      uint32_t depth;
    };

    struct SplitDelta
    {
      int x;
      int y;
      int z;
    };

    constexpr struct SplitSize
    {
      uint32_t x = 0;
      uint32_t y = 0;
      uint32_t z = 0;
    } cacheSplitCounts;

    struct CacheInitInfo
    {
      uint16_t count = 1;

      // -1 means uninitialized
      int32_t endXIndex = -1;
      int32_t endYIndex = -1;
      int32_t endZIndex = -1;

      uint16_t amountToInitialize = 1;
    };

    struct BoundingBox
    {
      using value_type = Position::value_type;

      BoundingBox(Position min, Position max)
          : _min(min), _max(max) {}

      const Position min() const noexcept;
      const Position max() const noexcept;

      value_type minX() const noexcept;
      value_type minY() const noexcept;
      value_type minZ() const noexcept;

      value_type maxX() const noexcept;
      value_type maxY() const noexcept;
      value_type maxZ() const noexcept;

      void setMinX(value_type value) noexcept;
      void setMinY(value_type value) noexcept;
      void setMinZ(value_type value) noexcept;

      void setMaxX(value_type value) noexcept;
      void setMaxY(value_type value) noexcept;
      void setMaxZ(value_type value) noexcept;

      /*
        Returns true if the bounding box changed.
      */
      bool include(BoundingBox box);
      /*
        Returns true if the bounding box changed.
      */
      bool include(const Position pos);

      bool contains(const BoundingBox &other) const noexcept;

    private:
      Position _min;
      Position _max;
    };

    static constexpr Cube ChunkSize = {64, 64, 8};
    constexpr uint32_t DefaultMaxCacheSplitCount = 4;

    constexpr CacheInitInfo computeCacheStuff(const vme::MapSize mapSize, const uint32_t maxCacheSplitCount = DefaultMaxCacheSplitCount)
    {
      SplitSize splitCounts;
      splitCounts.x = std::min(mapSize.width() / ChunkSize.width - 1, maxCacheSplitCount);
      splitCounts.y = std::min(mapSize.height() / ChunkSize.height - 1, maxCacheSplitCount);
      splitCounts.z = std::max(std::min(mapSize.depth() / ChunkSize.depth - 1, maxCacheSplitCount), static_cast<uint32_t>(0));

      CacheInitInfo cacheInfo;
      // There are one or zero chunks
      if (splitCounts.x == 0 && splitCounts.y == 0 && splitCounts.z == 0)
      {
        cacheInfo.count = 0;
        cacheInfo.amountToInitialize = 0;
        return cacheInfo;
      }

      int remainingDimensions = 3;

      do
      {
        int dimensions = 0;

        if (splitCounts.x > 0)
        {
          ++dimensions;
          --splitCounts.x;
        }

        if (splitCounts.y > 0)
        {
          ++dimensions;
          --splitCounts.y;
        }

        if (splitCounts.z > 0)
        {
          ++dimensions;
          --splitCounts.z;
        }

        cacheInfo.count *= power(2, dimensions);

        if (splitCounts.x == 0 && cacheInfo.endXIndex == -1)
          cacheInfo.endXIndex = cacheInfo.count;

        if (splitCounts.y == 0 && cacheInfo.endYIndex == -1)
          cacheInfo.endYIndex = cacheInfo.count;

        if (splitCounts.z == 0 && cacheInfo.endZIndex == -1)
          cacheInfo.endZIndex = cacheInfo.count;

      } while (splitCounts.x > 0 || splitCounts.y > 0 || splitCounts.z > 0);

      if (cacheInfo.endXIndex != cacheInfo.count)
        --remainingDimensions;
      if (cacheInfo.endYIndex != cacheInfo.count)
        --remainingDimensions;
      if (cacheInfo.endZIndex != cacheInfo.count)
        --remainingDimensions;

      cacheInfo.amountToInitialize = cacheInfo.count / power(2, remainingDimensions);
      return cacheInfo;
    }

    class HeapNode
    {
    public:
      HeapNode(HeapNode *parent) : parent(parent) {}
      virtual ~HeapNode() = default;
      virtual bool isLeaf() const;
      virtual bool isCachedNode() const noexcept
      {
        return false;
      }

      virtual void clear() = 0;

      virtual bool empty() const noexcept;

      Leaf *leaf(const Position pos) const;
      Leaf *getOrCreateLeaf(const Position pos);

      virtual std::string show() const;

      virtual HeapNode *child(int pattern) const;
      virtual HeapNode *child(const Position pos) const;
      virtual HeapNode *getOrCreateChild(const Position pos);

      virtual int childCount() const noexcept = 0;

      virtual bool contains(const Position pos) const;

      virtual uint16_t getIndex(const Position &pos) const = 0;

      virtual void updateBoundingBox()
      {
        VME_LOG_D("Warning: Called updateBoundingBox of virtual HeapNode.");
      }

      bool hasBoundingBox() const;

      const std::optional<BoundingBox> boundingBox() const;

      HeapNode *parent;
      std::variant<std::monostate, BoundingBox> _boundingBox;
    };

    class Leaf : public HeapNode
    {
    public:
      struct UpdateResult
      {
        bool change = false;
        bool bboxChange = false;

        template <size_t I>
        auto get() const
        {
          if constexpr (I == 0)
            return change;
          else if constexpr (I == 1)
            return bboxChange;
          else
            static_assert(I >= 0 && I < 1);
        }
      };

      Leaf(const Position pos, HeapNode *parent);
      bool isLeaf() const override;

      int childCount() const noexcept override;

      bool empty() const noexcept override;

      void clear() override;

      bool size() const noexcept;
      uint32_t count() const noexcept;

      /*
        Returns true if the leaf has the position 'pos'.
      */
      bool contains(const Position pos) const override;

      UpdateResult add(const Position pos);
      UpdateResult remove(const Position pos);

      bool encloses(const Position pos) const;

      uint16_t getIndex(const Position &pos) const override;

      std::string show() const override;

      Position min() const noexcept;
      Position max() const noexcept;

      std::array<bool, ChunkSize.width *ChunkSize.height *ChunkSize.depth> values = {false};

      Position position;

    private:
      uint32_t _count = 0;
      struct Indices
      {
        int x = -1;
        int y = -1;
        int z = -1;

#ifdef _DEBUG_VME
        bool validIndices() const noexcept
        {
          if (x == -1 || y == -1 || z == -1)
          {
            return x == -1 && y == -1 && z == -1;
          }

          return true;
        }
#endif

        bool empty() const
        {
          DEBUG_ASSERT(validIndices(), "Invalid octree indices!");
          return x == -1 || y == -1 || z == -1;
        }
      };
      /*
        Important: Indices hold indices into xs, ys, and zs. To get a position 
        based on low/high, use min() and max().
      */
      Indices low;
      Indices high;

      uint16_t xs[ChunkSize.width] = {0};
      uint16_t ys[ChunkSize.height] = {0};
      uint16_t zs[ChunkSize.depth] = {0};

      /*
        Returns true if the bounding box changed.
      */
      bool addToBoundingBox(const Position pos);

      /*
        Returns true if the bounding box changed.
      */
      bool removeFromBoundingBox(const Position pos);
    };

    template <size_t ChildCount>
    class BaseNode : public HeapNode
    {
    public:
      BaseNode(HeapNode *parent) : HeapNode(parent) {}

      HeapNode *child(int pattern) const override;
      HeapNode *child(const Position pos) const override;
      bool isLeaf() const override;
      void clear() override;
      int childCount() const noexcept override;

      void updateBoundingBox() override;

    protected:
      std::array<std::unique_ptr<HeapNode>, ChildCount> children;
    };

    DECLARE_NODE(NodeX, 2);
    DECLARE_NODE(NodeY, 2);
    DECLARE_NODE(NodeZ, 2);

    DECLARE_NODE(NodeXY, 4);
    DECLARE_NODE(NodeXZ, 4);
    DECLARE_NODE(NodeYZ, 4);

    DECLARE_NODE(NodeXYZ, 8);

    class CachedNode : public HeapNode
    {
    public:
      CachedNode(HeapNode *parent = nullptr);
      void setParent(HeapNode *parent);
      bool isCachedNode() const noexcept override;
      int childCount() const noexcept override;

      bool addBoundingBox(BoundingBox boundingBox);

      uint16_t childOffset(const int pattern) const;

      void updateBoundingBoxCached(const Tree &tree);

      void clear() override;

      int32_t childCacheOffset = -1;
      uint16_t cacheIndex = 0;
      uint8_t _childCount = 0;

      // void setIndex(size_t index);
      void setChildCacheOffset(size_t offset);
      uint16_t getIndex(const Position &pos) const override;
    };

    struct TraversalState
    {
      TraversalState(vme::MapSize mapSize, CacheInitInfo cacheInfo);
      Position pos;

      int update(uint16_t index, const Position position);

      int dx;
      int dy;
      int dz;

      CacheInitInfo cacheInfo;

      std::string show() const;
    };

    class Node;
    class Tree
    {
    public:
      static Tree create(const MapSize mapSize);
      Tree(vme::MapSize mapSize, CacheInitInfo cacheInfo);

      class leafIterator
      {
      public:
        using ValueType = Leaf *;
        using Reference = const ValueType &;
        using Pointer = Leaf *;
        using IteratorCategory = std::forward_iterator_tag;

        leafIterator();
        leafIterator(const Tree *tree);
        static leafIterator end();

        leafIterator &operator++();
        leafIterator operator++(int junk);

        const Leaf *operator*() const { return value; }
        const Leaf *operator->() const { return value; }

        bool operator==(const leafIterator &rhs) const;
        bool operator!=(const leafIterator &rhs) const { return !(*this == rhs); }

        bool finished() const noexcept;

      private:
        const Tree *tree;

        std::queue<const HeapNode *> nodes;

        const Leaf *value;

        bool isEnd = false;

        void nextLeaf();
      };

      class iterator
      {
      public:
        using ValueType = Position;
        using Reference = Position &;
        using Pointer = Position *;
        using IteratorCategory = std::forward_iterator_tag;

        iterator(const Tree *tree);
        static iterator end();

        iterator &operator++();
        iterator operator++(int junk);

        Reference operator*() { return value; }
        Pointer operator->() { return &value; }

        bool operator==(const iterator &rhs) const;
        bool operator!=(const iterator &rhs) const { return !(*this == rhs); }

      private:
        leafIterator leafIterator;

        Position value;

        Position topLeft;
        Position bottomRight;

        bool isEnd = false;

        void nextValue();

        const Leaf *currentLeaf() const;

        iterator();
      };

      iterator begin() const
      {
        return iterator(this);
      }

      iterator end() const
      {
        return iterator::end();
      }

      uint32_t width;
      uint32_t height;
      uint16_t floors;

      void add(const Position pos);
      void remove(const Position pos);

      /*
        Clear all positions from the tree.
      */
      void clear();

      bool contains(const Position pos) const;
      long size() const noexcept;
      bool empty() const noexcept;

      const std::optional<BoundingBox> boundingBox() const noexcept;

      // Position min() const noexcept;
      // Position max() const noexcept;

      // BoundingBox::value_type minX() const noexcept;
      // BoundingBox::value_type minY() const noexcept;
      // BoundingBox::value_type minZ() const noexcept;

      // BoundingBox::value_type maxX() const noexcept;
      // BoundingBox::value_type maxY() const noexcept;
      // BoundingBox::value_type maxZ() const noexcept;

      std::optional<Position> getCorner(bool positiveX, bool positiveY, bool positiveZ) const noexcept;

    private:
      friend class CachedNode;
      long _size = 0;

      // Should not change
      TraversalState initialState;

      CacheInitInfo cacheInfo;
      CachedNode root;

      mutable std::pair<CachedNode *, Leaf *> mostRecentLeaf;
      mutable std::vector<CachedNode> cachedNodes;

      std::vector<std::unique_ptr<HeapNode>> cachedHeapNodes;

      /*
        Indices into `cachedNodes` that have been accessed at some point. Used
        to implement a more efficient clear().
      */
      std::vector<uint16_t> usedCacheIndices;
      std::vector<uint16_t> usedHeapCacheIndices;

      HeapNode *getChild(int index, const HeapNode *node) const;

      void markAsRecent(CachedNode *cached, Leaf *leaf) const;

      void initializeCache();

      static std::unique_ptr<HeapNode> heapNodeFromSplitPattern(int pattern, const Position &pos, SplitDelta splitData, HeapNode *parent);

      const CachedNode *getCachedNode(const Position position) const;

      std::optional<std::pair<CachedNode *, HeapNode *>> fromCache(const Position position) const;
      std::pair<CachedNode *, HeapNode *> getOrCreateFromCache(const Position position);
      Leaf *getLeaf(const Position position) const;
      std::pair<CachedNode *, Leaf *> getOrCreateLeaf(const Position position);

    }; // End of Tree

    inline int Leaf::childCount() const noexcept
    {
      return 0;
    }

    inline bool Leaf::isLeaf() const
    {
      return true;
    }

    inline bool Leaf::size() const noexcept
    {
      return _count;
    }

    inline void Leaf::clear()
    {
      VME_LOG_D("Clear");
      values.fill(false);
    }

    inline bool Leaf::empty() const noexcept
    {
      return _count == 0;
    }

    inline Position Leaf::min() const noexcept
    {
      return position + Position(low.x, low.y, low.z);
    }

    inline Position Leaf::max() const noexcept
    {
      return position + Position(high.x, high.y, high.z);
    }

    template <size_t ChildCount>
    int BaseNode<ChildCount>::childCount() const noexcept
    {
      return ChildCount;
    }

    template <size_t ChildCount>
    inline bool BaseNode<ChildCount>::isLeaf() const
    {
      return false;
    }

    template <size_t ChildCount>
    inline HeapNode *BaseNode<ChildCount>::child(int index) const
    {
      DEBUG_ASSERT(index < ChildCount, "Bad pattern.");
      auto child = children[index].get();
      return child;
    }

    template <size_t ChildCount>
    inline HeapNode *BaseNode<ChildCount>::child(const Position pos) const
    {
      return children[getIndex(pos)].get();
    }

    template <size_t ChildCount>
    inline void BaseNode<ChildCount>::updateBoundingBox()
    {
      // VME_LOG_D("updateBoundingBox before: " << boundingBox);
      bool changed = false;
      _boundingBox = {};
      for (const std::unique_ptr<HeapNode> &node : children)
      {
        if (node && node->hasBoundingBox())
        {
          auto nodeBbox = node->boundingBox().value();
          if (!boundingBox())
          {
            _boundingBox = nodeBbox;
            changed = true;
          }
          else
          {
            BoundingBox &box = std::get<BoundingBox>(_boundingBox);
            changed = changed || box.include(nodeBbox);
          }
        }
      }
      // VME_LOG_D("updateBoundingBox after: " << boundingBox);

      if (!parent->isCachedNode() && changed)
        parent->updateBoundingBox();
    }

    template <size_t ChildCount>
    void BaseNode<ChildCount>::clear()
    {
      for (auto &c : children)
      {
        if (c)
          c->clear();
      }
    }

    inline std::optional<Position> Tree::getCorner(bool positiveX, bool positiveY, bool positiveZ) const noexcept
    {
      if (!root.boundingBox())
        return std::nullopt;

      auto &bbox = root.boundingBox().value();
      return Position(
          positiveX ? bbox.maxX() : bbox.minX(),
          positiveY ? bbox.maxY() : bbox.minY(),
          positiveZ ? bbox.maxZ() : bbox.minZ());
    }

    inline long Tree::size() const noexcept
    {
      return _size;
    }

    inline bool Tree::empty() const noexcept
    {
      return _size == 0;
    }

    inline const Position BoundingBox::min() const noexcept
    {
      return _min;
    }
    inline const Position BoundingBox::max() const noexcept
    {
      return _max;
    }

    inline BoundingBox::value_type BoundingBox::minX() const noexcept
    {
      return _min.x;
    }

    inline BoundingBox::value_type BoundingBox::minY() const noexcept
    {
      return _min.y;
    }

    inline BoundingBox::value_type BoundingBox::minZ() const noexcept
    {
      return _min.z;
    }

    inline BoundingBox::value_type BoundingBox::maxX() const noexcept
    {
      return _max.x;
    }

    inline BoundingBox::value_type BoundingBox::maxY() const noexcept
    {
      return _max.y;
    }

    inline BoundingBox::value_type BoundingBox::maxZ() const noexcept
    {
      return _max.z;
    }

    inline void BoundingBox::setMinX(value_type value) noexcept
    {
      _min.x = value;
    }

    inline void BoundingBox::setMinY(value_type value) noexcept
    {
      _min.y = value;
    }

    inline void BoundingBox::setMinZ(value_type value) noexcept
    {
      _min.z = value;
    }

    inline void BoundingBox::setMaxX(value_type value) noexcept
    {
      _max.x = value;
    }

    inline void BoundingBox::setMaxY(value_type value) noexcept
    {
      _max.y = value;
    }

    inline void BoundingBox::setMaxZ(value_type value) noexcept
    {
      _max.z = value;
    }

    // inline BoundingBox::value_type Tree::minX() const noexcept
    // {
    //   return boundingBox().minX();
    // }
    // inline BoundingBox::value_type Tree::minY() const noexcept
    // {
    //   return boundingBox().minY();
    // }
    // inline BoundingBox::value_type Tree::minZ() const noexcept
    // {
    //   return boundingBox().minZ();
    // }
    // inline BoundingBox::value_type Tree::maxX() const noexcept
    // {
    //   return boundingBox().maxX();
    // }
    // inline BoundingBox::value_type Tree::maxY() const noexcept
    // {
    //   return boundingBox().maxY();
    // }
    // inline BoundingBox::value_type Tree::maxZ() const noexcept
    // {
    //   return boundingBox().maxZ();
    // }

    // inline Position Tree::min() const noexcept
    // {
    //   return boundingBox().min();
    // }

    // inline Position Tree::max() const noexcept
    // {
    //   return boundingBox().max();
    // }

    inline std::unique_ptr<HeapNode> Tree::heapNodeFromSplitPattern(int pattern, const Position &midPoint, SplitDelta splitDelta, HeapNode *parent)
    {
      switch (pattern)
      {
      case 0: // If no more splits are possible, this cached node is a leaf node.
        return std::make_unique<Leaf>(midPoint, parent);
      case 0b1:
        return std::make_unique<NodeZ>(midPoint, splitDelta, parent);
      case 0b10:
        return std::make_unique<NodeY>(midPoint, splitDelta, parent);
      case 0b11:
        return std::make_unique<NodeYZ>(midPoint, splitDelta, parent);
      case 0b100:
        return std::make_unique<NodeX>(midPoint, splitDelta, parent);
      case 0b101:
        return std::make_unique<NodeXZ>(midPoint, splitDelta, parent);
      case 0b110:
        return std::make_unique<NodeXY>(midPoint, splitDelta, parent);
      case 0b111:
        return std::make_unique<NodeXYZ>(midPoint, splitDelta, parent);
      default:
        return {};
      }
    }

    inline bool HeapNode::isLeaf() const
    {
      return false;
    };

    inline std::string HeapNode::show() const
    {
      return "Unknown";
    }

    inline HeapNode *HeapNode::child(int pattern) const
    {
      return nullptr;
    }

    inline HeapNode *HeapNode::child(const Position pos) const
    {
      return nullptr;
    };

    inline HeapNode *HeapNode::getOrCreateChild(const Position pos)
    {
      return nullptr;
    }

    inline bool HeapNode::hasBoundingBox() const
    {
      return std::holds_alternative<BoundingBox>(_boundingBox);
    }

    inline const std::optional<BoundingBox> HeapNode::boundingBox() const
    {
      return std::holds_alternative<BoundingBox>(_boundingBox) ? std::optional<BoundingBox>(std::get<BoundingBox>(_boundingBox)) : std::nullopt;
    }

    inline bool HeapNode::empty() const noexcept
    {
      return std::holds_alternative<std::monostate>(_boundingBox);
    }

    inline uint32_t Leaf::count() const noexcept { return _count; }

    //>>>>>>>>>>>>>>>>>>>>>>>>>>>
    //>>>>>>>>>>>>>>>>>>>>>>>>>>>
    //>>>>>>>>>BoundingBox>>>>>>>
    //>>>>>>>>>>>>>>>>>>>>>>>>>>>
    //>>>>>>>>>>>>>>>>>>>>>>>>>>>
    inline bool operator==(const BoundingBox &l, const BoundingBox &r) noexcept
    {
      return l.min() == r.min() && l.max() == r.max();
    }

    inline bool operator!=(const BoundingBox &l, const BoundingBox &r) noexcept
    {
      return !(l == r);
    }

    inline std::ostream &operator<<(std::ostream &os, const BoundingBox &bbox)
    {
      os << "{ min: " << bbox.min() << ", max: " << bbox.max() << " }";
      return os;
    }

  } // namespace octree

} // namespace vme

STRUCTURED_BINDING(vme::octree::Leaf::UpdateResult, 2);