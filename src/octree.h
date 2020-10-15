#pragma once

#include <type_traits>
#include <memory>
#include <vector>
#include <array>
#include <sstream>
#include <limits>

#include "debug.h"
#include "util.h"
#include "position.h"

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

    struct BoundingBox
    {
      using value_type = Position::value_type;

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
      Position::value_type _top = std::numeric_limits<BoundingBox::value_type>::max();
      Position::value_type _right = std::numeric_limits<BoundingBox::value_type>::min();
      Position::value_type _bottom = std::numeric_limits<BoundingBox::value_type>::min();
      Position::value_type _left = std::numeric_limits<BoundingBox::value_type>::max();
    };

    struct Cube
    {
      uint32_t width;
      uint32_t height;
      uint32_t depth;
    };

    constexpr Cube MapSize = {4096, 4096, 16};
    constexpr Cube ChunkSize = {64, 64, 8};

    constexpr uint32_t MaxCacheSplitCount = 4;

    struct SplitDelta
    {
      int x;
      int y;
      int z;
    };

    constexpr struct SplitSize
    {
      uint32_t x = std::min(MapSize.width / ChunkSize.width - 1, MaxCacheSplitCount);
      uint32_t y = std::min(MapSize.height / ChunkSize.height - 1, MaxCacheSplitCount);
      uint32_t z = std::max(std::min(MapSize.depth / ChunkSize.depth - 1, MaxCacheSplitCount), static_cast<uint32_t>(0));
    } cacheSplitCounts;

    constexpr struct MaxSplit
    {
      uint32_t x = MapSize.width / ChunkSize.width - 1;
      uint32_t y = MapSize.height / ChunkSize.height - 1;
      uint32_t z = MapSize.depth / ChunkSize.depth - 1;
    } MaxSplit;

    struct CacheData
    {
      uint16_t count = 1;
      int32_t endXIndex = -1;
      int32_t endYIndex = -1;
      int32_t endZIndex = -1;
      uint16_t amountToInitialize = 1;
    };

    constexpr CacheData computeCachedNodeCount()
    {
      SplitSize splitCounts = cacheSplitCounts;
      CacheData cacheData;
      // There are one or zero chunks
      if (splitCounts.x == 0 && splitCounts.y == 0 && splitCounts.z == 0)
        return cacheData;

      int initModifier = 3;

      int exponent = 0;
      do
      {
        exponent = 0;

        if (splitCounts.x > 0)
        {
          ++exponent;
          --splitCounts.x;
        }

        if (splitCounts.y > 0)
        {
          ++exponent;
          --splitCounts.y;
        }

        if (splitCounts.z > 0)
        {
          ++exponent;
          --splitCounts.z;
        }

        cacheData.count *= power(2, exponent);

        if (splitCounts.x == 0 && cacheData.endXIndex == -1)
          cacheData.endXIndex = cacheData.count;
        if (splitCounts.y == 0 && cacheData.endYIndex == -1)
          cacheData.endYIndex = cacheData.count;
        if (splitCounts.z == 0 && cacheData.endZIndex == -1)
          cacheData.endZIndex = cacheData.count;
      } while (splitCounts.x > 0 || splitCounts.y > 0 || splitCounts.z > 0);

      if (cacheData.endXIndex != cacheData.count)
        --initModifier;
      if (cacheData.endYIndex != cacheData.count)
        --initModifier;
      if (cacheData.endZIndex != cacheData.count)
        --initModifier;

      cacheData.amountToInitialize = cacheData.count / power(2, initModifier);
      return cacheData;
    }
    constexpr CacheData CachedNodeCount = computeCachedNodeCount();

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
      Leaf(const Position pos, HeapNode *parent);
      bool isLeaf() const override;

      bool contains(const Position pos) override;
      bool add(const Position pos);

      uint16_t getIndex(const Position &pos) const override;

      std::string show() const override;

      bool values[ChunkSize.width * ChunkSize.height * ChunkSize.depth] = {false};
      Position position;
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

    public:
      friend class Tree;

      int32_t childCacheOffset = -1;
      uint16_t cacheIndex = 0;

      // void setIndex(size_t index);
      void setChildCacheOffset(size_t offset);
      uint16_t getIndex(const Position &pos) const override;
    };

    class Node;
    class Tree
    {
    public:
      Tree(uint32_t width, uint32_t height, uint16_t floors);

      uint32_t width;
      uint32_t height;
      uint16_t floors;

      struct TraversalState
      {
        TraversalState(Position pos, int dx, int dy, int dz) : pos(pos), dx(dx), dy(dy), dz(dz) {}
        Position pos;

        int update(Position p);

        int dx;
        int dy;
        int dz;

        std::string show() const
        {
          std::ostringstream s;
          s << "(" << pos.x << ", " << pos.y << ", " << pos.z << "), deltas: (" << dx << ", " << dy << ", " << dz << ")";
          return s.str();
        }
      };

      // std::array<Node,

      void add(const Position pos);
      bool contains(const Position pos) const;

    private:
      friend class CachedNode;
      // Should not change
      TraversalState top;

      CachedNode root;
      std::array<CachedNode, CachedNodeCount.amountToInitialize> cachedNodes;
      std::array<std::unique_ptr<HeapNode>, CachedNodeCount.count - CachedNodeCount.amountToInitialize> cachedHeapNodes;

      static std::unique_ptr<HeapNode> heapNodeFromSplitPattern(int pattern, const Position &pos, SplitDelta splitData, HeapNode *parent);

      HeapNode *fromCache(const Position position) const;
      std::pair<CachedNode *, HeapNode *> getOrCreateFromCache(const Position position);
      Leaf *leaf(const Position position) const;
      Leaf *getOrCreateLeaf(const Position position);

    }; // End of Tree

    inline bool Leaf::isLeaf() const
    {
      return true;
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
      VME_LOG_D("updateBoundingBox before: " << boundingBox);
      auto p = this;

      boundingBox.reset();
      for (const std::unique_ptr<HeapNode> &node : children)
      {
        if (node)
        {
          auto nodeBbox = node->boundingBox;
          boundingBox.include(nodeBbox);
        }
      }
      VME_LOG_D("updateBoundingBox after: " << boundingBox);
      VME_LOG_D("updateBoundingBox after: " << boundingBox);

      if (!parent->isCachedNode())
        parent->updateBoundingBox(BoundingBox());
    }

    //>>>>>>>>>>>>>>>>>>>>>
    //>>>>>>>>Node>>>>>>>>>
    //>>>>>>>>>>>>>>>>>>>>>

    // factory function
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