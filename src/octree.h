#pragma once

#include <type_traits>
#include <memory>
#include <vector>
#include <array>
#include <sstream>

#include "position.h"

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

    struct Cube
    {
      uint32_t width;
      uint32_t height;
      uint32_t depth;
    };

    constexpr Cube MapSize = {128, 128, 16};
    constexpr Cube ChunkSize = {64, 64, 8};

    constexpr uint32_t MaxSplits = 4;

    constexpr struct SplitSize
    {
      uint32_t x = std::min(MapSize.width / ChunkSize.width - 1, MaxSplits);
      uint32_t y = std::min(MapSize.height / ChunkSize.height - 1, MaxSplits);
      uint32_t z = std::max(std::min(MapSize.depth / ChunkSize.depth - 1, MaxSplits), static_cast<uint32_t>(0));
    } splitSizes;

    constexpr struct MaxSplit
    {
      uint32_t x = MapSize.width / ChunkSize.width - 1;
      uint32_t y = MapSize.height / ChunkSize.height - 1;
      uint32_t z = MapSize.depth / ChunkSize.depth - 1;
    } MaxSplit;

    struct CacheData
    {
      uint16_t count = 1;
      int32_t endXIndex = 0;
      int32_t endYIndex = 0;
      int32_t endZIndex = 0;
      uint16_t amountToInitialize = 1;
    };

    constexpr CacheData computeCachedNodeCount()
    {
      SplitSize sizes = splitSizes;
      CacheData cacheData;
      // There are one or zero chunks
      if (sizes.x == 0 && sizes.y == 0 && sizes.z == 0)
        return cacheData;

      int initModifier = 3;

      if (sizes.x <= MaxSplits)
      {
        --initModifier;
        cacheData.endXIndex = -1;
      }
      if (sizes.y <= MaxSplits)
      {
        --initModifier;
        cacheData.endYIndex = -1;
      }
      if (sizes.z <= MaxSplits)
      {
        --initModifier;
        cacheData.endZIndex = -1;
      }

      int exponent = 0;
      do
      {
        exponent = 0;

        if (sizes.x > 0)
        {
          ++exponent;
          --sizes.x;
        }

        if (sizes.y > 0)
        {
          ++exponent;
          --sizes.y;
        }

        if (sizes.z > 0)
        {
          ++exponent;
          --sizes.z;
        }

        cacheData.count *= power(2, exponent);

        if (sizes.x == 0 && cacheData.endXIndex == 0)
        {
          --initModifier;
          cacheData.endXIndex = cacheData.count;
        }
        if (sizes.y == 0 && cacheData.endYIndex == 0)
        {
          --initModifier;
          cacheData.endYIndex = cacheData.count;
        }
        if (sizes.z == 0 && cacheData.endZIndex == 0)
        {
          --initModifier;
          cacheData.endZIndex = cacheData.count;
        }
      } while (sizes.x > 0 || sizes.y > 0 || sizes.z > 0);

      cacheData.amountToInitialize = cacheData.count * (1 - 1 / power(2, initModifier));
      return cacheData;
    }
    constexpr CacheData CachedNodeCount = computeCachedNodeCount();

    class Node;
    class Tree
    {
    public:
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
        TraversalState(int x, int y, int z, int dx, int dy, int dz) : x(x), y(y), z(z), dx(dx), dy(dy), dz(dz) {}
        int x;
        int y;
        int z;

        int update(Position p);

        int dx;
        int dy;
        int dz;

        std::string show() const
        {
          std::ostringstream s;
          s << "(" << x << ", " << y << ", " << z << "), deltas: (" << dx << ", " << dy << ", " << dz << ")";
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

      class Leaf;
      class HeapNode
      {
      public:
        virtual ~HeapNode() = default;

      protected:
        virtual bool isLeaf() const = 0;
        virtual Leaf *leaf(const Position pos) const = 0;
        virtual Leaf *getOrCreateLeaf(const Position pos) = 0;

        virtual HeapNode *child(int pattern) const
        {
          return nullptr;
        }
        virtual HeapNode *getOrCreateChild(int pattern)
        {
          return nullptr;
        }

        virtual void add(const Position pos) = 0;
      };

      class Leaf : public HeapNode
      {
      public:
        Leaf(const Position pos);
        bool isLeaf() const override;
        Leaf *leaf(const Position pos) const override;
        Leaf *getOrCreateLeaf(const Position pos) override;

        void add(const Position pos) override;

        Position position;
        bool values[ChunkSize.width * ChunkSize.height * ChunkSize.depth] = {{0}};
      };

      template <size_t ChildrenCount>
      class Node : public HeapNode
      {
      public:
        Node(const SplitInfo split);
        bool isLeaf() const override;
        Leaf *leaf(const Position pos) const override;
        Leaf *getOrCreateLeaf(const Position pos) override;

        HeapNode *child(int pattern) const override;
        HeapNode *getOrCreateChild(int pattern) override;

        void add(const Position pos) override;

        std::array<std::unique_ptr<HeapNode>, ChildrenCount> children;
      };

      CachedNode root;
      std::array<CachedNode, CachedNodeCount.count> cachedNodes;
      std::array<std::unique_ptr<HeapNode>, CachedNodeCount.count - CachedNodeCount.amountToInitialize> cachedHeapNodes;

      // bits 0bxyz
      int dimensionPattern;

    }; // End of Tree

    // inline bool Tree::NodeXY::isLeaf() const
    // {
    //   return false;
    // }

    inline bool Tree::Leaf::isLeaf() const
    {
      return true;
    }

    template <size_t ChildrenCount>
    Tree::Node<ChildrenCount>::Node(const SplitInfo split) : split(split) {}

    template <size_t ChildrenCount>
    Tree::HeapNode *Tree::Node<ChildrenCount>::child(int pattern) const
    {
      DEBUG_ASSERT((pattern & ~(0b111)) == 0, "Bad pattern.");
      auto child = children[pattern].get();
      return child;
    }

    template <size_t ChildrenCount>
    Tree::HeapNode *Tree::Node<ChildrenCount>::getOrCreateChild(int pattern)
    {
      // auto child = children[pattern];
      // if (child)
      //   return child;

      return nullptr;
    }

    template <size_t ChildrenCount>
    Tree::Leaf *Tree::Node<ChildrenCount>::leaf(const Position pos) const
    {
      return nullptr;
    }

    template <size_t ChildrenCount>
    Tree::Leaf *Tree::Node<ChildrenCount>::getOrCreateLeaf(const Position pos)
    {
      return nullptr;
    }

    template <size_t ChildrenCount>
    void Tree::Node<ChildrenCount>::add(Position pos)
    {
      getOrCreateLeaf(pos)->add(pos);
    }

    template <size_t ChildrenCount>
    inline bool Tree::Node<ChildrenCount>::isLeaf() const
    {
      return false;
    }

    //>>>>>>>>>>>>>>>>>>>>>>>>>
    //>>>>>>>>>>>>>>>>>>>>>>>>>
    //>>>>>>>>>>Show>>>>>>>>>>>
    //>>>>>>>>>>>>>>>>>>>>>>>>>
    //>>>>>>>>>>>>>>>>>>>>>>>>>

  } // namespace octree

} // namespace vme