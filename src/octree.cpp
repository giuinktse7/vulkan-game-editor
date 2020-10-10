#include "octree.h"

#include "debug.h"
#include "position.h"

namespace vme
{
  namespace octree
  {
    Tree::Tree(uint32_t width, uint32_t height, uint16_t floors)
        : width(width), height(height), floors(floors),
          top(width / 2, height / 2, floors / 2, width / 4, height / 4, floors / 4)
    {
      // int base = 0;
      root.childBase = 0;
      // root.children._000 = base;
      // root.children._001 = base + 1;
      // root.children._010 = base + 2;
      // root.children._011 = base + 3;
      // root.children._100 = base + 4;
      // root.children._101 = base + 5;
      // root.children._110 = base + 6;
      // root.children._111 = base + 7;
      // root.index = -2;

      for (int i = 0; i < cachedNodes.size() / 8; ++i)
        cachedNodes[i].setIndex(i);
    }

    void Tree::add(const Position pos)
    {
      TraversalState state = top;

      auto &cached = root;
      while (cached.childBase != -1)
      {
        // x y z
        int pattern = state.update(pos);
        cached = cachedNodes[cached.child(pattern)];
        VME_LOG_D(cached.childBase);
      }

      VME_LOG_D("Final state: " << state.show());
    }

    int Tree::TraversalState::update(Position pos)
    {
      int pattern = 0;
      if (pos.x > x)
      {
        pattern |= (1 << 2);
        x += dx;
      }
      else
        x -= dx;

      if (pos.y > y)
      {
        pattern |= (1 << 1);
        y += dy;
      }
      else
        y -= dy;

      if (pos.z > z)
      {
        pattern |= (1 << 0);
        z += dz;
      }
      else
        z -= dz;

      dx /= 2;
      dy /= 2;
      dz /= 2;

      return pattern;
    }

    Tree::Node::Node()
    {
    }

    Tree::CachedNode::CachedNode() {}

    uint16_t Tree::CachedNode::child(const int pattern)
    {
      return childBase + pattern;
      // switch (pattern)
      // {
      // case 0b0:
      //   return children._000;
      // case 0b1:
      //   return children._001;
      // case 0b10:
      //   return children._010;
      // case 0b11:
      //   return children._011;
      // case 0b100:
      //   return children._100;
      // case 0b101:
      //   return children._101;
      // case 0b110:
      //   return children._110;
      // case 0b111:
      //   return children._111;
      // }
    }

    void Tree::CachedNode::setIndex(size_t index)
    {
      // this->index = index;
      childBase = (index + 1) * 8;
      // auto base = (index + 1) * 8;
      // children._000 = base;
      // children._001 = base + 1;
      // children._010 = base + 2;
      // children._011 = base + 3;
      // children._100 = base + 4;
      // children._101 = base + 5;
      // children._110 = base + 6;
      // children._111 = base + 7;
    }

    Tree::HeapNode *Tree::NonLeaf::child(const int pattern) const
    {
      return children[pattern];
    }

    Tree::HeapNode &Tree::NonLeaf::getOrCreateChild(const int pattern)
    {
      auto child = children[pattern];
      if (child)
        return *child;
    }

  } // namespace octree
} // namespace vme
