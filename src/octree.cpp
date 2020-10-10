#include "octree.h"

#include "debug.h"

namespace vme
{
  namespace octree
  {
    Tree::Tree(uint32_t width, uint32_t height, uint16_t floors)
        : width(width), height(height), floors(floors),
          top(width / 2, height / 2, floors / 2, width / 4, height / 4, floors / 4),
          dimensionPattern(0b111)
    {
      root.childCacheOffset = 0;

      if (CachedNodeCount.endXIndex == -1)
        dimensionPattern &= (~0b100);
      if (CachedNodeCount.endYIndex == -1)
        dimensionPattern &= (~0b010);
      if (CachedNodeCount.endZIndex == -1)
        dimensionPattern &= (~0b001);

      int exponent = 0;
      if (CachedNodeCount.endXIndex != 0)
        ++exponent;
      if (CachedNodeCount.endYIndex != 0)
        ++exponent;
      if (CachedNodeCount.endZIndex != 0)
        ++exponent;

      int offset = power(2, exponent);
      for (int i = 0; i < CachedNodeCount.amountToInitialize; ++i)
      {
        cachedNodes[i].childCacheOffset = (i + 1) * offset;
        if (i == CachedNodeCount.endXIndex)
          offset /= 2;
        if (i == CachedNodeCount.endYIndex)
          offset /= 2;
        if (i == CachedNodeCount.endZIndex)
          offset /= 2;
      }
    }

    void Tree::add(const Position pos)
    {
      TraversalState state = top;

      SplitInfo split;

      auto &cached = root;
      uint16_t index = 0;
      while (cached.childCacheOffset != -1)
      {
        // x y z
        int pattern = state.update(pos);
        split.update(pattern);

        index = cached.child(pattern);
        cached = cachedNodes[index];
        VME_LOG_D(cached.childCacheOffset);
      }
      VME_LOG_D("Final cache state: " << state.show());
      VME_LOG_D(index);

      // If no more splits are possible, this cached node is a leaf node.
      if (dimensionPattern == 0)
      {
        Leaf *leaf;
        if (!cachedHeapNodes.at(index))
        {
          cachedHeapNodes.at(index) = std::make_unique<Leaf>(pos);
        }
        leaf = static_cast<Leaf *>(cachedHeapNodes.at(index).get());

        VME_LOG_D("Leaf node: " << leaf->position);
      }
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

    void Tree::SplitInfo::update(int pattern)
    {
      DEBUG_ASSERT((pattern & (~0b111)) == 0, "Bad pattern.");

      x += (pattern >> 2) & 1;
      y += (pattern >> 1) & 1;
      z += pattern & 1;
    }

    Tree::CachedNode::CachedNode() {}

    uint16_t Tree::CachedNode::child(const int pattern)
    {
      return childCacheOffset + pattern;
    }

    void Tree::CachedNode::setChildCacheOffset(size_t offset)
    {
      childCacheOffset = offset;
    }

    // Tree::HeapNode *Tree::NodeXY::child(int pattern) const
    // {
    //   DEBUG_ASSERT((pattern & (~0b11)) == 0, "Bad pattern.");
    //   auto child = children[pattern].get();
    //   return child;
    // }

    // void Tree::NodeXY::add(const Position pos)
    // {
    //   Leaf *leaf = getOrCreateLeaf(pos);
    // }

    //>>>>>>>>>>>>>>>>>>>>>
    //>>>>>>>>Leaf>>>>>>>>>
    //>>>>>>>>>>>>>>>>>>>>>

    Tree::Leaf::Leaf(const Position pos)
        : position(pos.x / ChunkSize.width, pos.y / ChunkSize.height, pos.z / ChunkSize.depth) {}

    Tree::Leaf *Tree::Leaf::leaf(const Position pos) const
    {
      return const_cast<Tree::Leaf *>(this);
    }

    Tree::Leaf *Tree::Leaf::getOrCreateLeaf(const Position pos)
    {
      return this;
    }

    void Tree::Leaf::add(const Position pos)
    {
      Position delta = pos - this->position;
      auto index = (delta.x * ChunkSize.height + delta.y) * ChunkSize.depth + delta.z;
      values[index] = true;
    }

  } // namespace octree
} // namespace vme
