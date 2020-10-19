#pragma once

#include <type_traits>
#include <memory>
#include <optional>
#include <vector>
#include <array>
#include <sstream>
#include <limits>

#include "debug.h"
#include "position.h"
#include "time_point.h"

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
      BoundingBox() {}
      BoundingBox(value_type top, value_type right, value_type bottom, value_type left)
          : _top(top), _right(right), _bottom(bottom), _left(left) {}

      value_type top() const noexcept;
      value_type right() const noexcept;
      value_type bottom() const noexcept;
      value_type left() const noexcept;

      void setTop(value_type value) noexcept;
      void setRight(value_type value) noexcept;
      void setBottom(value_type value) noexcept;
      void setLeft(value_type value) noexcept;

      void reset();

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
      value_type _top = std::numeric_limits<BoundingBox::value_type>::max();
      value_type _right = std::numeric_limits<BoundingBox::value_type>::min();
      value_type _bottom = std::numeric_limits<BoundingBox::value_type>::min();
      value_type _left = std::numeric_limits<BoundingBox::value_type>::max();
    };

    static constexpr Cube ChunkSize = {64, 64, 8};
    constexpr uint32_t DefaultMaxCacheSplitCount = 4;

    constexpr CacheInitInfo computeCacheStuff(const Cube mapSize, const uint32_t maxCacheSplitCount = DefaultMaxCacheSplitCount)
    {
      SplitSize splitCounts;
      splitCounts.x = std::min(mapSize.width / ChunkSize.width - 1, maxCacheSplitCount);
      splitCounts.y = std::min(mapSize.height / ChunkSize.height - 1, maxCacheSplitCount);
      splitCounts.z = std::max(std::min(mapSize.depth / ChunkSize.depth - 1, maxCacheSplitCount), static_cast<uint32_t>(0));

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

      Leaf *leaf(const Position pos) const;
      Leaf *getOrCreateLeaf(const Position pos);

      virtual std::string show() const;

      virtual HeapNode *child(int pattern) const;
      virtual HeapNode *child(const Position pos) const;
      virtual HeapNode *getOrCreateChild(const Position pos);

      virtual bool contains(const Position pos);

      virtual uint16_t getIndex(const Position &pos) const = 0;

      virtual void updateBoundingBox(BoundingBox bbox)
      {
        VME_LOG_D("Warning: Called updateBoundingBox of virtual HeapNode.");
      }

      HeapNode *parent;
      BoundingBox boundingBox;
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

      void clear() override;

      uint32_t count() const noexcept;

      /*
        Returns true if the leaf has the position 'pos'.
      */
      bool contains(const Position pos) override;

      /*
        Returns true if the bounding box changed.
      */
      UpdateResult add(const Position pos);

      /*
        Returns true if the bounding box changed.
      */
      UpdateResult remove(const Position pos);

      bool encloses(const Position pos) const;

      uint16_t getIndex(const Position &pos) const override;

      std::string show() const override;

      std::array<bool, ChunkSize.width *ChunkSize.height *ChunkSize.depth> values = {false};

      Position position;

    private:
      uint32_t _count = 0;
      struct Indices
      {
        int x, y, z = -1;
      };
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

      bool empty() const noexcept
      {
        return _count == 0;
      }
    };

    template <size_t ChildCount>
    class BaseNode : public HeapNode
    {
    public:
      BaseNode(HeapNode *parent) : HeapNode(parent) {}
      size_t childCount = ChildCount;

      HeapNode *child(int pattern) const override;
      HeapNode *child(const Position pos) const override;
      bool isLeaf() const override;
      void clear() override;

      void updateBoundingBox(BoundingBox bbox) override;

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

      uint16_t childOffset(const int pattern) const;

      void updateBoundingBoxCached(const Tree &tree);

      void clear() override;

    public:
      int32_t childCacheOffset = -1;
      uint16_t cacheIndex = 0;

      // void setIndex(size_t index);
      void setChildCacheOffset(size_t offset);
      uint16_t getIndex(const Position &pos) const override;
    };

    struct TraversalState
    {
      TraversalState(Cube mapSize, CacheInitInfo cacheInfo);
      Position pos;

      int update(uint16_t index, const Position position);
      int update2(const Position position);

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
      static Tree create(const Cube mapSize);

      Tree(Cube mapSize, CacheInitInfo cacheInfo);
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

      BoundingBox boundingBox() const noexcept;
      Position topLeft() const noexcept;
      Position topRight() const noexcept;
      Position bottomRight() const noexcept;
      Position bottomLeft() const noexcept;

    private:
      friend class CachedNode;
      long _size = 0;

      mutable std::pair<CachedNode *, Leaf *> mostRecentLeaf;

      // Should not change
      TraversalState top;

      CacheInitInfo cacheInfo;

      CachedNode root;
      std::vector<CachedNode> cachedNodes;
      std::vector<std::unique_ptr<HeapNode>> cachedHeapNodes;

      /*
        Indices into `cachedNodes` that have been accessed at some point. Used
        to implement a more efficient clear().
      */
      std::vector<uint16_t> usedCacheIndices;
      std::vector<uint16_t> usedHeapCacheIndices;

      void markAsRecent(CachedNode *cached, Leaf *leaf) const;

      void initializeCache();

      static std::unique_ptr<HeapNode> heapNodeFromSplitPattern(int pattern, const Position &pos, SplitDelta splitData, HeapNode *parent);

      /*
        Returns the index of the cached heap node containing the position.
      */
      uint16_t cachedHeapNodeIndex(const Position position) const;

      const CachedNode *getCachedNode(const Position position) const;

      std::optional<std::pair<CachedNode *, HeapNode *>> fromCache(const Position position) const;
      std::pair<CachedNode *, HeapNode *> getOrCreateFromCache(const Position position);
      Leaf *getLeaf(const Position position) const;
      std::pair<CachedNode *, Leaf *> getOrCreateLeaf(const Position position);

    }; // End of Tree

    inline bool Leaf::isLeaf() const
    {
      return true;
    }

    inline void Leaf::clear()
    {
      values.fill(false);
    }

    template <size_t ChildCount>
    inline bool BaseNode<ChildCount>::isLeaf() const
    {
      return false;
    }

    template <size_t ChildCount>
    inline HeapNode *BaseNode<ChildCount>::child(int index) const
    {
      DEBUG_ASSERT(index < childCount, "Bad pattern.");
      auto child = children[index].get();
      return child;
    }

    template <size_t ChildCount>
    inline HeapNode *BaseNode<ChildCount>::child(const Position pos) const
    {
      return children[getIndex(pos)].get();
    }

    template <size_t ChildCount>
    inline void BaseNode<ChildCount>::updateBoundingBox(BoundingBox bbox)
    {
      // VME_LOG_D("updateBoundingBox before: " << boundingBox);
      boundingBox.reset();
      for (const std::unique_ptr<HeapNode> &node : children)
      {
        if (node)
        {
          auto nodeBbox = node->boundingBox;
          boundingBox.include(nodeBbox);
        }
      }
      // VME_LOG_D("updateBoundingBox after: " << boundingBox);

      if (!parent->isCachedNode())
        parent->updateBoundingBox(BoundingBox());
    }

    template <size_t ChildCount>
    void BaseNode<ChildCount>::clear()
    {
      for (auto &c : children)
        c->clear();
    }

    inline long Tree::size() const noexcept
    {
      return _size;
    }

    inline bool Tree::empty() const noexcept
    {
      return _size == 0;
    }

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

    inline uint32_t Leaf::count() const noexcept { return _count; }

    //>>>>>>>>>>>>>>>>>>>>>>>>>>>
    //>>>>>>>>>>>>>>>>>>>>>>>>>>>
    //>>>>>>>>>BoundingBox>>>>>>>
    //>>>>>>>>>>>>>>>>>>>>>>>>>>>
    //>>>>>>>>>>>>>>>>>>>>>>>>>>>
    inline bool operator==(const BoundingBox &l, const BoundingBox &r) noexcept
    {
      return l.top() == r.top() && l.right() == r.right() && l.bottom() == r.bottom() && l.left() == r.left();
    }

    inline bool operator!=(const BoundingBox &l, const BoundingBox &r) noexcept
    {
      return !(l == r);
    }

    inline BoundingBox::value_type BoundingBox::top() const noexcept
    {
      return _top;
    }

    inline BoundingBox::value_type BoundingBox::right() const noexcept
    {
      return _right;
    }

    inline BoundingBox::value_type BoundingBox::bottom() const noexcept
    {
      return _bottom;
    }

    inline BoundingBox::value_type BoundingBox::left() const noexcept
    {
      return _left;
    }

    inline void BoundingBox::setTop(BoundingBox::value_type value) noexcept
    {
      _top = value;
    }

    inline void BoundingBox::setRight(BoundingBox::value_type value) noexcept
    {
      _right = value;
    }

    inline void BoundingBox::setBottom(BoundingBox::value_type value) noexcept
    {
      _bottom = value;
    }

    inline void BoundingBox::setLeft(BoundingBox::value_type value) noexcept
    {
      _left = value;
    }

    inline std::ostream &operator<<(std::ostream &os, const BoundingBox &bbox)
    {
      os << "{ top: " << bbox.top() << ", right: " << bbox.right() << ", bottom: " << bbox.bottom() << ", left: " << bbox.left() << " }";
      return os;
    }

  } // namespace octree

} // namespace vme

STRUCTURED_BINDING(vme::octree::Leaf::UpdateResult, 2);

#undef VME_OCTREE_TREE_TEMPLATE