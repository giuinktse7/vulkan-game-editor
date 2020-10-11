#include "octree.h"

#include "debug.h"

namespace vme
{
  namespace octree
  {
    Tree::Tree(uint32_t width, uint32_t height, uint16_t floors)
        : width(width), height(height), floors(floors),
          top(Position(width / 2, height / 2, floors / 2), width / 4, height / 4, floors / 4),
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
      Tree::TraversalState state = top;

      SplitInfo split;

      auto &cached = root;
      uint16_t index = 0;
      int pattern = 0;
      while (cached.childCacheOffset != -1)
      {
        // x y z
        pattern = state.update(pos);
        split.update(pattern);

        index = cached.child(pattern);
        cached = cachedNodes[index];
        VME_LOG_D("Offset: " << cached.childCacheOffset);
      }
      VME_LOG_D("Final cache state: " << state.show());
      VME_LOG_D(index);

      SplitDelta splitDelta{state.dx, state.dy, state.dz};

      if (splitDelta.x <= ChunkSize.width)
        pattern &= 1 << 2;
      if (splitDelta.y <= ChunkSize.height)
        pattern &= 1 << 1;
      if (splitDelta.z <= ChunkSize.depth)
        pattern &= 1 << 0;

      if (!cachedHeapNodes.at(index))
        cachedHeapNodes.at(index) = Tree::heapNodeFromSplitPattern(pattern, state.pos, splitDelta);

      auto node = cachedHeapNodes.at(index).get();
      if (node)
      {
        VME_LOG_D("Have node.");
        if (node->isLeaf())
        {
          auto leaf = static_cast<Leaf *>(node);
          leaf->add(pos);
          VME_LOG_D("Leaf node: " << leaf->position);
        }
      }
    }

    int Tree::TraversalState::update(Position pos)
    {
      int pattern = 0;
      if (pos.x > this->pos.x)
      {
        pattern |= (1 << 2);
        this->pos.x += dx;
      }
      else
        this->pos.x -= dx;

      if (pos.y > this->pos.y)
      {
        pattern |= (1 << 1);
        this->pos.y += dy;
      }
      else
        this->pos.y -= dy;

      if (pos.z > this->pos.z)
      {
        pattern |= (1 << 0);
        this->pos.z += dz;
      }
      else
        this->pos.z -= dz;

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

    //>>>>>>>>>>>>>>>>>>>>>>>>>
    //>>>>>>>>HeapNode>>>>>>>>>
    //>>>>>>>>>>>>>>>>>>>>>>>>>

    Tree::Leaf *Tree::HeapNode::leaf(const Position pos) const
    {
      HeapNode *node = const_cast<Tree::HeapNode *>(this);
      while (!node->isLeaf())
        node = node->child(node->index(pos));

      return static_cast<Tree::Leaf *>(node);
    }

    bool Tree::HeapNode::contains(const Position pos)
    {
      return false;
    }

    void Tree::HeapNode::add(const Position pos)
    {
      getOrCreateLeaf(pos)->add(pos);
    }

    //>>>>>>>>>>>>>>>>>>>>>
    //>>>>>>>>Leaf>>>>>>>>>
    //>>>>>>>>>>>>>>>>>>>>>

    Tree::Leaf::Leaf(const Position pos)
        : position(pos.x - pos.x % ChunkSize.width, pos.y - pos.y % ChunkSize.height, pos.z - pos.z % ChunkSize.depth) {}

    bool Tree::Leaf::contains(const Position pos)
    {
      auto index = ((pos.x - position.x) * ChunkSize.height + (pos.y - position.y)) * ChunkSize.depth + (pos.z - position.z);

      return values[index];
    }

    void Tree::Leaf::add(const Position pos)
    {
      DEBUG_ASSERT(
          (position.x <= pos.x && pos.x < position.x + ChunkSize.width) &&
              (position.y <= pos.y && pos.y < position.y + ChunkSize.height) &&
              (position.z <= pos.z && pos.z < position.z + ChunkSize.depth),
          "The position does not belong to this chunk.");

      values[index(pos)] = true;
    }

    Tree::Leaf *Tree::HeapNode::getOrCreateLeaf(const Position pos)
    {
      if (isLeaf())
        return static_cast<Tree::Leaf *>(this);

      int childIndex = index(pos);

      auto child = getOrCreateChild(childIndex);
      while (!child->isLeaf())
        child = child->getOrCreateChild(child->index(pos));

      return static_cast<Tree::Leaf *>(child);
    }

    //>>>>>>>>>>>>>>>>>>>>>>
    //>>>>>>>>NodeZ>>>>>>>>>
    //>>>>>>>>>>>>>>>>>>>>>>
    Tree::NodeZ::NodeZ(const Position midPoint, const SplitDelta splitDelta)
        : midPoint(midPoint), splitDelta(splitDelta) {}

    Tree::HeapNode *Tree::NodeZ::child(int pattern) const
    {
      DEBUG_ASSERT((pattern & ~(0b1)) == 0, "Bad pattern.");
      auto child = children[pattern].get();
      return child;
    }

    Tree::HeapNode *Tree::NodeZ::child(const Position pos) const
    {
      return children[index(pos)].get();
    }

    Tree::HeapNode *Tree::NodeZ::getOrCreateChild(int index)
    {
      if (children[index])
        return children[index].get();

      SplitDelta delta = splitDelta;
      delta.z >>= 2;

      Position pos = midPoint;
      pos.z += index == 0 ? -delta.z : delta.z;

      if (delta.z >= ChunkSize.depth)
        children[index] = std::make_unique<NodeZ>(pos, delta);
      else // Child
        children[index] = std::make_unique<Leaf>(pos);

      return children[index].get();
    }

    inline uint16_t Tree::NodeZ::index(const Position &pos) const
    {
      return pos.z > this->midPoint.z;
    }

    //>>>>>>>>>>>>>>>>>>>>>>
    //>>>>>>>>NodeY>>>>>>>>>
    //>>>>>>>>>>>>>>>>>>>>>>
    Tree::NodeY::NodeY(const Position midPoint, const SplitDelta splitDelta)
        : midPoint(midPoint), splitDelta(splitDelta) {}

    Tree::HeapNode *Tree::NodeY::child(int pattern) const
    {
      DEBUG_ASSERT((pattern & ~(0b1)) == 0, "Bad pattern.");
      auto child = children[pattern].get();
      return child;
    }

    Tree::HeapNode *Tree::NodeY::child(const Position pos) const
    {
      return children[index(pos)].get();
    }

    Tree::HeapNode *Tree::NodeY::getOrCreateChild(int index)
    {
      if (children[index])
        return children[index].get();

      SplitDelta delta = splitDelta;
      delta.y >>= 2;

      Position pos = midPoint;
      pos.y += index == 0 ? -delta.y : delta.y;

      if (delta.y >= ChunkSize.height)
        children[index] = std::make_unique<NodeY>(pos, delta);
      else // Child
        children[index] = std::make_unique<Leaf>(pos);

      return children[index].get();
    }

    inline uint16_t Tree::NodeY::index(const Position &pos) const
    {
      return pos.y > this->midPoint.y;
    }

    //>>>>>>>>>>>>>>>>>>>>>>
    //>>>>>>>>NodeX>>>>>>>>>
    //>>>>>>>>>>>>>>>>>>>>>>
    Tree::NodeX::NodeX(const Position midPoint, const SplitDelta splitDelta)
        : midPoint(midPoint), splitDelta(splitDelta) {}

    Tree::HeapNode *Tree::NodeX::child(int pattern) const
    {
      DEBUG_ASSERT((pattern & ~(0b1)) == 0, "Bad pattern.");
      auto child = children[pattern].get();
      return child;
    }

    Tree::HeapNode *Tree::NodeX::child(const Position pos) const
    {
      return children[index(pos)].get();
    }

    Tree::HeapNode *Tree::NodeX::getOrCreateChild(int index)
    {
      if (children[index])
        return children[index].get();

      SplitDelta delta = splitDelta;
      delta.x >>= 2;

      Position pos = midPoint;
      pos.x += index == 0 ? -delta.x : delta.x;

      if (delta.x >= ChunkSize.width)
        children[index] = std::make_unique<NodeX>(pos, delta);
      else // Child
        children[index] = std::make_unique<Leaf>(pos);

      return children[index].get();
    }

    inline uint16_t Tree::NodeX::index(const Position &pos) const
    {
      return pos.x > this->midPoint.x;
    }

    //>>>>>>>>>>>>>>>>>>>>>>
    //>>>>>>>>NodeXY>>>>>>>>>
    //>>>>>>>>>>>>>>>>>>>>>>

    Tree::NodeXY::NodeXY(const Position midPoint, const SplitDelta splitDelta)
        : midPoint(midPoint), splitDelta(splitDelta) {}

    Tree::HeapNode *Tree::NodeXY::child(int pattern) const
    {
      DEBUG_ASSERT((pattern & ~(0b11)) == 0, "Bad pattern.");
      auto child = children[pattern].get();
      return child;
    }

    Tree::HeapNode *Tree::NodeXY::child(const Position pos) const
    {
      return children[index(pos)].get();
    }

    inline Tree::HeapNode *Tree::NodeXY::getOrCreateChild(int index)
    {
      if (children[index])
        return children[index].get();

      SplitDelta delta = splitDelta;
      delta.x >>= 2;
      delta.y >>= 2;

      Position pos = midPoint;
      pos.x += (index >> 1) == 0 ? -delta.x : delta.x;
      pos.y += index == 0 ? -delta.y : delta.y;

      int split = 0;
      split |= (delta.x >= ChunkSize.width) << 1;
      split |= (delta.y >= ChunkSize.height);
      switch (split)
      {
      case 0:
        children[index] = std::make_unique<Leaf>(pos);
        break;
      case 0b1:
        children[index] = std::make_unique<NodeY>(pos, delta);
        break;
      case 0b10:
        children[index] = std::make_unique<NodeX>(pos, delta);
        break;
      case 0b11:
        children[index] = std::make_unique<NodeXY>(pos, delta);
        break;
      }

      return children[index].get();
    }

    inline uint16_t Tree::NodeXY::index(const Position &pos) const
    {
      return ((pos.x > midPoint.x) << 1) + (pos.y > midPoint.y);
    }

    //>>>>>>>>>>>>>>>>>>>>>>
    //>>>>>>>>NodeXZ>>>>>>>>>
    //>>>>>>>>>>>>>>>>>>>>>>

    Tree::NodeXZ::NodeXZ(const Position midPoint, const SplitDelta splitDelta)
        : midPoint(midPoint), splitDelta(splitDelta) {}

    Tree::HeapNode *Tree::NodeXZ::child(int pattern) const
    {
      DEBUG_ASSERT((pattern & ~(0b11)) == 0, "Bad pattern.");
      auto child = children[pattern].get();
      return child;
    }

    Tree::HeapNode *Tree::NodeXZ::child(const Position pos) const
    {
      return children[index(pos)].get();
    }

    inline Tree::HeapNode *Tree::NodeXZ::getOrCreateChild(int index)
    {
      if (children[index])
        return children[index].get();

      SplitDelta delta = splitDelta;
      delta.x >>= 2;
      delta.z >>= 2;

      Position pos = midPoint;
      pos.x += (index >> 1) == 0 ? -delta.x : delta.x;
      pos.z += index == 0 ? -delta.z : delta.z;

      int split = 0;
      split |= (delta.x >= ChunkSize.width) << 1;
      split |= (delta.z >= ChunkSize.depth);
      switch (split)
      {
      case 0:
        children[index] = std::make_unique<Leaf>(pos);
        break;
      case 0b1:
        children[index] = std::make_unique<NodeZ>(pos, delta);
        break;
      case 0b10:
        children[index] = std::make_unique<NodeX>(pos, delta);
        break;
      case 0b11:
        children[index] = std::make_unique<NodeXZ>(pos, delta);
        break;
      }

      return children[index].get();
    }

    uint16_t Tree::NodeXZ::index(const Position &pos) const
    {
      return ((pos.x > midPoint.x) << 1) + (pos.z > midPoint.z);
    }

    //>>>>>>>>>>>>>>>>>>>>>>
    //>>>>>>>>NodeYZ>>>>>>>>>
    //>>>>>>>>>>>>>>>>>>>>>>

    Tree::NodeYZ::NodeYZ(const Position midPoint, const SplitDelta splitDelta)
        : midPoint(midPoint), splitDelta(splitDelta) {}

    Tree::HeapNode *Tree::NodeYZ::child(int pattern) const
    {
      DEBUG_ASSERT((pattern & ~(0b11)) == 0, "Bad pattern.");
      auto child = children[pattern].get();
      return child;
    }

    Tree::HeapNode *Tree::NodeYZ::child(const Position pos) const
    {
      return children[index(pos)].get();
    }

    Tree::HeapNode *Tree::NodeYZ::getOrCreateChild(int index)
    {
      if (children[index])
        return children[index].get();

      SplitDelta delta = splitDelta;
      delta.y >>= 2;
      delta.z >>= 2;

      Position pos = midPoint;
      pos.y += (index >> 1) == 0 ? -delta.y : delta.y;
      pos.z += index == 0 ? -delta.z : delta.z;

      int split = 0;
      split |= (delta.y >= ChunkSize.height) << 1;
      split |= (delta.z >= ChunkSize.depth);
      switch (split)
      {
      case 0:
        children[index] = std::make_unique<Leaf>(pos);
        break;
      case 0b1:
        children[index] = std::make_unique<NodeZ>(pos, delta);
        break;
      case 0b10:
        children[index] = std::make_unique<NodeY>(pos, delta);
        break;
      case 0b11:
        children[index] = std::make_unique<NodeYZ>(pos, delta);
        break;
      }

      return children[index].get();
    }

    inline uint16_t Tree::NodeYZ::index(const Position &pos) const
    {
      return ((pos.y > midPoint.y) << 1) + (pos.z > midPoint.z);
    }

    //>>>>>>>>>>>>>>>>>>>>>>>>
    //>>>>>>>>NodeXYZ>>>>>>>>>
    //>>>>>>>>>>>>>>>>>>>>>>>>

    Tree::NodeXYZ::NodeXYZ(const Position midPoint, const SplitDelta splitDelta)
        : midPoint(midPoint), splitDelta(splitDelta) {}

    Tree::HeapNode *Tree::NodeXYZ::child(int pattern) const
    {
      DEBUG_ASSERT((pattern & ~(0b111)) == 0, "Bad pattern.");
      auto child = children[pattern].get();
      return child;
    }

    Tree::HeapNode *Tree::NodeXYZ::child(const Position pos) const
    {
      return children[index(pos)].get();
    }

    inline Tree::HeapNode *Tree::NodeXYZ::getOrCreateChild(int index)
    {
      if (children[index])
        return children[index].get();

      SplitDelta delta = splitDelta;
      delta.x >>= 2;
      delta.y >>= 2;
      delta.z >>= 2;

      Position pos = midPoint;
      pos.x += (index >> 2) == 0 ? -delta.x : delta.x;
      pos.y += (index >> 1) == 0 ? -delta.y : delta.y;
      pos.z += index == 0 ? -delta.z : delta.z;

      int split = 0;
      split |= (delta.x >= ChunkSize.width) << 2;
      split |= (delta.y >= ChunkSize.height) << 1;
      split |= (delta.y >= ChunkSize.height);
      switch (split)
      {
      case 0:
        children[index] = std::make_unique<Leaf>(pos);
        break;
      case 0b1:
        children[index] = std::make_unique<NodeZ>(pos, delta);
        break;
      case 0b10:
        children[index] = std::make_unique<NodeY>(pos, delta);
        break;
      case 0b11:
        children[index] = std::make_unique<NodeYZ>(pos, delta);
        break;
      case 0b100:
        children[index] = std::make_unique<NodeX>(pos, delta);
        break;
      case 0b101:
        children[index] = std::make_unique<NodeXZ>(pos, delta);
        break;
      case 0b110:
        children[index] = std::make_unique<NodeXY>(pos, delta);
        break;
      case 0b111:
        children[index] = std::make_unique<NodeXYZ>(pos, delta);
        break;
      }

      return children[index].get();
    }

    inline uint16_t Tree::NodeXYZ::index(const Position &pos) const
    {
      return ((pos.x > midPoint.x) << 2) | ((pos.y > midPoint.y) << 1) | (pos.z > midPoint.z);
    }

  } // namespace octree
} // namespace vme
