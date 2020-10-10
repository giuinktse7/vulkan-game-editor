#pragma once

#include <vector>
#include <array>
#include <sstream>

struct Position;

namespace vme
{
  namespace octree
  {
    template <typename T>
    constexpr T power(T num, uint32_t pow)
    {
      T result = 1;
      for (; pow > 1; --pow)
        result *= num;

      return result;
    }

    constexpr uint32_t ChildCount = 8;
    constexpr uint32_t MapWidth = 1024;
    constexpr uint32_t MapHeight = 1024;
    constexpr uint32_t MapZ = 16;
    constexpr uint16_t CachedNodeCount = power(ChildCount, 4) * 2; // 8192

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

        // x y z
        // struct
        // {
        //   uint16_t _000;
        //   uint16_t _001;
        //   uint16_t _010;
        //   uint16_t _011;
        //   uint16_t _100;
        //   uint16_t _101;
        //   uint16_t _110;
        //   uint16_t _111;
        // } children;

        int childBase = -1;

        void setIndex(size_t index);
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

      CachedNode root;
      std::array<CachedNode, CachedNodeCount> cachedNodes;
      // std::array<Node,

      void add(const Position pos);

    private:
      // Should not change
      TraversalState top;

      class HeapNode
      {
        virtual ~HeapNode() = default;

        virtual bool leaf() const = 0;
      };

      class NonLeaf : public HeapNode
      {
      public:
        NonLeaf();
        bool leaf() const override;

        HeapNode *child(const int pattern) const;
        HeapNode &getOrCreateChild(const int pattern);

        std::array<HeapNode *, ChildCount> children;
      };

      class Leaf : public HeapNode
      {
      public:
        bool leaf() const override;

        bool values[32][32][4] = {{0}};
      };
    };

    inline bool Tree::NonLeaf::leaf() const
    {
      return false;
    }

    inline bool Tree::Leaf::leaf() const
    {
      return true;
    }

  } // namespace octree

} // namespace vme