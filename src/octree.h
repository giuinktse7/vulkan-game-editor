#pragma once

#include <type_traits>
#include <memory>
#include <vector>
#include <array>
#include <sstream>

#include "debug.h"
#include "util.h"
#include "position.h"

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
    template <typename T>
    constexpr T power(T num, uint32_t pow)
    {
      T result = 1;
      for (; pow >= 1; --pow)
        result *= num;

      return result;
    }

    constexpr size_t test(int d)
    {
      int exponent = (d & 1) + (d & 0b10) + (d & 0b100);
      if (exponent == 0)
        return 0;
      return power(2, exponent);
    }

    struct Cube
    {
      uint32_t width;
      uint32_t height;
      uint32_t depth;
    };

    enum ChildDimensions
    {
      None = 0,
      X = 1 << 0,
      Y = 1 << 1,
      Z = 1 << 2
    };

    // constexpr Cube MapSize = {128, 128, 16};

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

    class Node;
    class Tree
    {
    public:
      class Leaf;

      class HeapNode
      {
      public:
        virtual ~HeapNode() = default;
        virtual bool isLeaf() const
        {
          return false;
        };

        Leaf *leaf(const Position pos) const;
        Leaf *getOrCreateLeaf(const Position pos);

        virtual std::string show() const
        {
          return "Unknown";
        }

        virtual HeapNode *child(int pattern) const
        {
          return nullptr;
        }
        virtual HeapNode *child(const Position pos) const
        {
          return nullptr;
        };
        virtual HeapNode *getOrCreateChild(const Position pos)
        {
          return nullptr;
        }

        virtual bool contains(const Position pos);
        virtual void add(const Position pos);

        virtual uint16_t getIndex(const Position &pos) const = 0;
      };

      class Leaf : public HeapNode
      {
      public:
        Leaf(const Position pos);
        bool isLeaf() const override;

        bool contains(const Position pos) override;
        void add(const Position pos) override;

        uint16_t getIndex(const Position &pos) const override
        {
          return 0;
        }

        std::string show() const override;

        Position position;
        bool values[ChunkSize.width * ChunkSize.height * ChunkSize.depth] = {false};
      };

      template <size_t ChildCount>
      class BaseNode : public HeapNode
      {
      public:
        size_t childCount = ChildCount;

        HeapNode *child(int pattern) const override;
        HeapNode *child(const Position pos) const override;
        bool isLeaf() const override;

      protected:
        std::array<std::unique_ptr<HeapNode>, ChildCount> children;
      };

#define DECLARE_NODE(name, amount)                              \
  class name : public BaseNode<amount>                          \
  {                                                             \
  public:                                                       \
    name(const Position midPoint, const SplitDelta splitDelta); \
                                                                \
    HeapNode *getOrCreateChild(const Position pos) override;    \
    std::string show() const override;                          \
                                                                \
  private:                                                      \
    Position midPoint;                                          \
    SplitDelta splitDelta;                                      \
                                                                \
    uint16_t getIndex(const Position &pos) const override;      \
  }

      DECLARE_NODE(NodeX, 2);
      DECLARE_NODE(NodeY, 2);
      DECLARE_NODE(NodeZ, 2);

      DECLARE_NODE(NodeXY, 4);
      DECLARE_NODE(NodeXZ, 4);
      DECLARE_NODE(NodeYZ, 4);

      DECLARE_NODE(NodeXYZ, 8);

      class CachedNode
      {
      public:
        CachedNode();

        uint16_t child(const int pattern);

      public:
        friend class Tree;

        int32_t childCacheOffset = -1;

        // void setIndex(size_t index);
        void setChildCacheOffset(size_t offset);
      };

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

    private:
      // Should not change
      TraversalState top;

      struct SplitInfo
      {
        uint8_t x = 0;
        uint8_t y = 0;
        uint8_t z = 0;

        void update(int pattern);

        inline bool isEdge() const
        {
          return x == MaxSplit.x || y == MaxSplit.y || z == MaxSplit.z;
        }

        std::string show() const
        {
          std::ostringstream os;
          os << "Split {" << static_cast<int>(x) << ", " << static_cast<int>(y) << ", " << static_cast<int>(z) << "}";
          return os.str();
        }
      };

      CachedNode root;
      std::array<CachedNode, CachedNodeCount.count> cachedNodes;
      std::array<std::unique_ptr<HeapNode>, CachedNodeCount.count - CachedNodeCount.amountToInitialize> cachedHeapNodes;

      static std::unique_ptr<HeapNode> heapNodeFromSplitPattern(int pattern, const Position &pos, SplitDelta splitData);

    }; // End of Tree

    inline bool Tree::Leaf::isLeaf() const
    {
      return true;
    }

    template <size_t ChildCount>
    inline bool Tree::BaseNode<ChildCount>::isLeaf() const
    {
      return false;
    }

    template <size_t ChildCount>
    inline Tree::HeapNode *Tree::BaseNode<ChildCount>::child(int index) const
    {
      DEBUG_ASSERT(index < childCount, "Bad pattern.");
      auto child = children[index].get();
      return child;
    }

    template <size_t ChildCount>
    inline Tree::HeapNode *Tree::BaseNode<ChildCount>::child(const Position pos) const
    {
      return children[getIndex(pos)].get();
    }

    //>>>>>>>>>>>>>>>>>>>>>
    //>>>>>>>>Node>>>>>>>>>
    //>>>>>>>>>>>>>>>>>>>>>

    // factory function
    inline std::unique_ptr<Tree::HeapNode> Tree::heapNodeFromSplitPattern(int pattern, const Position &midPoint, SplitDelta splitDelta)
    {
      std::unique_ptr<HeapNode> ptr;
      switch (pattern)
      {
      case 0: // If no more splits are possible, this cached node is a leaf node.
        return std::make_unique<Leaf>(midPoint);
      case 0b1:
        ptr = std::make_unique<NodeZ>(midPoint, splitDelta);
        return ptr;
      case 0b10:
        return std::make_unique<NodeY>(midPoint, splitDelta);
      case 0b11:
        return std::make_unique<NodeYZ>(midPoint, splitDelta);
      case 0b100:
        return std::make_unique<NodeX>(midPoint, splitDelta);
      case 0b101:
        return std::make_unique<NodeXZ>(midPoint, splitDelta);
      case 0b110:
        return std::make_unique<NodeXY>(midPoint, splitDelta);
      case 0b111:
        return std::make_unique<NodeXYZ>(midPoint, splitDelta);
      default:
        return {};
      }
    }

    //>>>>>>>>>>>>>>>>>>>>>>>>>>
    //>>>>>>>>>>index>>>>>>>>>>>
    //>>>>>>>>>>>>>>>>>>>>>>>>>>
    // inline uint16_t Tree::NodeZ::index(const Position &pos) const
    // {
    // int i = 0;
    // switch (dimensions)
    // {
    // case ChildDimensions::X:
    //   i |= (pos.x > midPoint.x);
    //   break;
    // case ChildDimensions::Y:
    //   i |= (pos.y > midPoint.y);
    //   break;
    // case ChildDimensions::Z:
    //   i |= (pos.z > midPoint.z);
    //   break;
    // default:
    //   ABORT_PROGRAM("Bad dimension (= None)");
    // }

    // return i;
    // }

    // uint16_t Tree::Node<2>::index(const Position &pos) const
    // {
    //   int i = 0;
    //   if (dimensions & (ChildDimensions::X | ChildDimensions::Y))
    //     i |= ((pos.x > midPoint.x) << 1) + (pos.y > midPoint.y);
    //   else if (dimensions & (ChildDimensions::X | ChildDimensions::Z))
    //     i |= ((pos.x > midPoint.x) << 1) + (pos.z > midPoint.z);
    //   else // YZ
    //     i |= ((pos.y > midPoint.y) << 1) + (pos.z > midPoint.z);

    //   return i;
    // }

    // template <>
    // uint16_t Tree::Node<3>::index(const Position &pos) const
    // {
    //   return ((pos.x > midPoint.x) << 2) | ((pos.y > midPoint.y) << 1) | (pos.z > midPoint.z);
    // }

    //>>>>>>>>>>>>>>>>>>>>>>>>>
    //>>>>>>>>>>>>>>>>>>>>>>>>>
    //>>>>>>>>>>Show>>>>>>>>>>>
    //>>>>>>>>>>>>>>>>>>>>>>>>>
    //>>>>>>>>>>>>>>>>>>>>>>>>>

  } // namespace octree

} // namespace vme