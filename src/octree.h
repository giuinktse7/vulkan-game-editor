#pragma once

#include <type_traits>
#include <memory>
#include <optional>
#include <vector>
#include <array>
#include <sstream>
#include <limits>

#include "debug.h"
#include "util.h"
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
    template <uint16_t CacheSize, uint16_t CacheInitAmount>
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

      /*
        Returns true if the bounding box changed.
      */
      bool add(const Position pos);

      /*
        Returns true if the bounding box changed.
      */
      bool remove(const Position pos);

      uint16_t getIndex(const Position &pos) const override;

      std::string show() const override;

      bool values[ChunkSize.width * ChunkSize.height * ChunkSize.depth] = {false};

      Position position;

    private:
      uint32_t count = 0;
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
        return count == 0;
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
      template <uint16_t CacheSize, uint16_t CacheInitAmount>
      void updateBoundingBoxCached(const Tree<CacheSize, CacheInitAmount> &tree);

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
    template <uint16_t CacheSize, uint16_t CacheInitAmount>
    class Tree
    {
    public:
      constexpr Tree(Cube mapSize, CacheInitInfo cacheInfo);
      uint32_t width;
      uint32_t height;
      uint16_t floors;

      void add(const Position pos);
      void remove(const Position pos);
      bool contains(const Position pos) const;

      BoundingBox boundingBox() const noexcept;

    private:
      friend class CachedNode;
      // Should not change
      TraversalState top;

      CacheInitInfo cacheInfo;

      CachedNode root;
      std::array<CachedNode, CacheInitAmount> cachedNodes;
      std::array<std::unique_ptr<HeapNode>, CacheSize - CacheInitAmount + 8> cachedHeapNodes;

      static std::unique_ptr<HeapNode> heapNodeFromSplitPattern(int pattern, const Position &pos, SplitDelta splitData, HeapNode *parent);

      /*
        Returns the index of the cached heap node containing the position.
      */
      uint16_t cachedHeapNodeIndex(const Position position) const;
      HeapNode *fromCache(const Position position) const;
      std::pair<CachedNode *, HeapNode *> getOrCreateFromCache(const Position position);
      Leaf *leaf(const Position position) const;
      Leaf *getOrCreateLeaf(const Position position);

    }; // End of Tree

    //>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
    //>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
    //>>>>>>>>>Tree Implementation>>>>>>>
    //>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
    //>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

    template <uint16_t CacheSize, uint16_t CacheInitAmount>
    constexpr Tree<CacheSize, CacheInitAmount>::Tree(Cube mapSize, CacheInitInfo cacheInfo)
        : width(mapSize.width), height(mapSize.height), floors(mapSize.depth),
          top(mapSize, cacheInfo),
          cacheInfo(cacheInfo)
    {
      root.childCacheOffset = 0;

      int exponent = 0;
      if (cacheInfo.endXIndex != 0)
        ++exponent;
      if (cacheInfo.endYIndex != 0)
        ++exponent;
      if (cacheInfo.endZIndex != 0)
        ++exponent;

      int offset = power(2, exponent);

      // Set parent for root children
      for (int i = 0; i < offset; ++i)
        cachedNodes[i].setParent(&root);

      int cursor = offset;
      for (int i = 0; i < cacheInfo.amountToInitialize; ++i)
      {
        cachedNodes[i].cacheIndex = i;
        cachedNodes[i].childCacheOffset = cursor;

        if (cursor == cacheInfo.endXIndex)
          offset /= 2;
        if (cursor == cacheInfo.endYIndex)
          offset /= 2;
        if (cursor == cacheInfo.endZIndex)
          offset /= 2;

        auto lastChild = std::min<uint16_t>(cursor + offset, cacheInfo.amountToInitialize);

        for (int k = cursor; k < lastChild; ++k)
          cachedNodes[k].setParent(&cachedNodes[i]);

        cursor += offset;
      }
    }

    template <uint16_t CacheSize, uint16_t CacheInitAmount>
    BoundingBox Tree<CacheSize, CacheInitAmount>::boundingBox() const noexcept
    {
      return root.boundingBox;
    }

    template <uint16_t CacheSize, uint16_t CacheInitAmount>
    uint16_t Tree<CacheSize, CacheInitAmount>::cachedHeapNodeIndex(const Position position) const
    {
      TraversalState state = top;
      const CachedNode *cached = &root;

      int childIndex = state.update(cached->childCacheOffset, position);
      uint16_t cacheIndex = cached->childOffset(childIndex);
      cached = &cachedNodes[cacheIndex];

      while (true)
      {
        childIndex = state.update(cached->childCacheOffset, position);
        cacheIndex = cached->childOffset(childIndex);

        if (cacheIndex >= cachedNodes.size())
          break;

        cached = &cachedNodes[cacheIndex];
      }

      return cacheIndex - cacheInfo.amountToInitialize;
    }

    template <uint16_t CacheSize, uint16_t CacheInitAmount>
    HeapNode *Tree<CacheSize, CacheInitAmount>::fromCache(const Position position) const
    {
      uint16_t index = cachedHeapNodeIndex(position);
      auto &result = cachedHeapNodes.at(index);

      return result ? result.get() : nullptr;
    }

    template <uint16_t CacheSize, uint16_t CacheInitAmount>
    std::pair<CachedNode *, HeapNode *> Tree<CacheSize, CacheInitAmount>::getOrCreateFromCache(const Position position)
    {
      // VME_LOG_D("getOrCreateFromCache: " << position);
      TraversalState state = top;
      CachedNode *cached = &root;
      // auto k = this;

      // auto test = [position](TraversalState &state) {
      //   VME_LOG_D(": " << (state.pos.x < position.x) << " " << (state.pos.y < position.y) << " " << (state.pos.z < position.z));
      // };

      // VME_LOG_D("Start: " << state.pos);

      int childIndex;
      uint16_t cacheIndex;
      while (true)
      {
        // test(state);
        childIndex = state.update(cached->childCacheOffset, position);
        cacheIndex = cached->childOffset(childIndex);
        // VME_LOG_D("cache: " << cacheIndex << ", child: " << childIndex << " (" << state.pos << ")");

        if (cacheIndex >= cachedNodes.size())
          break;

        cached = &cachedNodes[cacheIndex];
      }
      uint16_t cacheHeapIndex = cacheIndex - cacheInfo.amountToInitialize;
      // VME_LOG_D("heap: " << cacheHeapIndex);

      int nodeType = 0;
      if (state.dx >= ChunkSize.width)
        nodeType |= (1 << 2);
      if (state.dy >= ChunkSize.height)
        nodeType |= (1 << 1);
      if (state.dz >= ChunkSize.depth)
        nodeType |= (1 << 0);

      // For leaf node
      if (nodeType == 0)
        state.pos = position;

      if (!cachedHeapNodes.at(cacheHeapIndex))
        cachedHeapNodes.at(cacheHeapIndex) = heapNodeFromSplitPattern(
            nodeType,
            state.pos,
            SplitDelta{state.dx, state.dy, state.dz},
            cached);

      return {cached, cachedHeapNodes.at(cacheHeapIndex).get()};
    }

    template <uint16_t CacheSize, uint16_t CacheInitAmount>
    bool Tree<CacheSize, CacheInitAmount>::contains(const Position pos) const
    {
      auto l = leaf(pos);
      return l && l->contains(pos);
    }

    static long us = 0;

    template <uint16_t CacheSize, uint16_t CacheInitAmount>
    void Tree<CacheSize, CacheInitAmount>::add(const Position pos)
    {
      auto [cached, node] = getOrCreateFromCache(pos);
      auto l = node->getOrCreateLeaf(pos);

      bool changed = l->add(pos);
      if (changed)
        cached->updateBoundingBoxCached(*this);
    }

    template <uint16_t CacheSize, uint16_t CacheInitAmount>
    void Tree<CacheSize, CacheInitAmount>::remove(const Position pos)
    {
      auto [cached, node] = getOrCreateFromCache(pos);
      auto l = node->getOrCreateLeaf(pos);

      bool changed = l->remove(pos);
      if (changed)
        cached->updateBoundingBoxCached(*this);
    }

    template <uint16_t CacheSize, uint16_t CacheInitAmount>
    Leaf *Tree<CacheSize, CacheInitAmount>::leaf(const Position position) const
    {
      return fromCache(position)->leaf(position);
    }

    template <uint16_t CacheSize, uint16_t CacheInitAmount>
    Leaf *Tree<CacheSize, CacheInitAmount>::getOrCreateLeaf(const Position position)
    {
      return fromCache(position)->getOrCreateLeaf(position);
    }

    //>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
    //>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
    //>>>>End of Tree Implementation>>>>>
    //>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
    //>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

    template <uint16_t CacheSize, uint16_t CacheInitAmount>
    void CachedNode::updateBoundingBoxCached(const Tree<CacheSize, CacheInitAmount> &tree)
    {
      // VME_LOG_D("updateBoundingBoxCached before: " << boundingBox);
      boundingBox.reset();

      int splits = 0;
      if (cacheIndex < tree.cacheInfo.endXIndex)
        ++splits;
      if (cacheIndex < tree.cacheInfo.endYIndex)
        ++splits;
      if (cacheIndex < tree.cacheInfo.endZIndex)
        ++splits;

      DEBUG_ASSERT(splits != 0, "Update bounding box for leaf node? Bug?");

      int childCount = power(2, splits);
      for (int i = 0; i < childCount; ++i)
      {
        if (childCacheOffset >= tree.cacheInfo.amountToInitialize) // Get from HeapNode cache
        {
          auto &node = tree.cachedHeapNodes.at(childCacheOffset + i - tree.cacheInfo.amountToInitialize);
          if (node)
            boundingBox.include(node->boundingBox);
        }
        else // Get from CachedNode cache
        {
          auto node = tree.cachedNodes.at(childCacheOffset + i);
          boundingBox.include(node.boundingBox);
        }
      }

      // VME_LOG_D("updateBoundingBoxCached after: " << boundingBox);
      if (parent)
        static_cast<CachedNode *>(parent)->updateBoundingBoxCached(tree);
    }

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

    //>>>>>>>>>>>>>>>>>>>>>
    //>>>>>>>>Node>>>>>>>>>
    //>>>>>>>>>>>>>>>>>>>>>

    // factory function
    template <uint16_t CacheSize, uint16_t CacheInitAmount>
    inline std::unique_ptr<HeapNode> Tree<CacheSize, CacheInitAmount>::heapNodeFromSplitPattern(int pattern, const Position &midPoint, SplitDelta splitDelta, HeapNode *parent)
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

#undef VME_OCTREE_TREE_TEMPLATE