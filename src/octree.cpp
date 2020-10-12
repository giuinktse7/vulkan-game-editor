#include "octree.h"

#include "debug.h"

namespace vme
{
  namespace octree
  {
    Tree::TraversalState initialTraversalState(uint32_t width, uint32_t height, uint16_t floors)
    {
      Position pos(
          std::max(ChunkSize.width, width / 2),
          std::max(ChunkSize.height, height / 2),
          std::max<Position::value_type>(ChunkSize.depth, floors / 2));

      uint32_t dx = std::max(ChunkSize.width, width / 4);
      uint32_t dy = std::max(ChunkSize.height, height / 4);
      uint32_t dz = std::max<Position::value_type>(ChunkSize.depth, floors / 4);

      return Tree::TraversalState(pos, dx, dy, dz);
    }

    Tree::Tree(uint32_t width, uint32_t height, uint16_t floors)
        : width(width), height(height), floors(floors),
          top(initialTraversalState(width, height, floors))
    {
      root.childCacheOffset = 0;

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

      auto &cached = root;
      uint16_t index = 0;
      int pattern = 0;
      while (cached.childCacheOffset != -1)
      {
        // x y z
        pattern = state.update(pos);
        VME_LOG_D("pattern: " << pattern);

        index = cached.child(pattern);
        cached = cachedNodes[index];
        VME_LOG_D("Offset: " << cached.childCacheOffset);
      }
      VME_LOG_D("Final cache state: " << state.show());
      VME_LOG_D(index);

      // state.update(pos);

      SplitDelta splitDelta{state.dx, state.dy, state.dz};

      int currentPattern = 0;
      if (state.dx >= ChunkSize.width)
        currentPattern |= (1 << 2);
      if (state.dy >= ChunkSize.height)
        currentPattern |= (1 << 1);
      if (state.dz >= ChunkSize.depth)
        currentPattern |= (1 << 0);

      if (!cachedHeapNodes.at(index))
        cachedHeapNodes.at(index) = Tree::heapNodeFromSplitPattern(currentPattern, state.pos, splitDelta);

      auto node = cachedHeapNodes.at(index).get();
      if (node)
      {
        VME_LOG_D("Have node.");
        if (node->isLeaf())
        {
          VME_LOG_D("Cached node was leaf.");
          auto leaf = static_cast<Leaf *>(node);
          leaf->add(pos);
          VME_LOG_D("Leaf node: " << leaf->position);
        }
        else
        {
          // TODO test this
          VME_LOG_D("Cached node was not leaf.");
          auto leaf = node->getOrCreateLeaf(pos);
          leaf->add(pos);
          VME_LOG_D("Leaf node (non-cached): " << leaf->position);
        }
      }
    }

    int Tree::TraversalState::update(Position pos)
    {
      int shiftX = (dz >= ChunkSize.depth / 2) + (dy >= ChunkSize.height / 2);
      int shiftY = shiftX - 1;
      int shiftZ = shiftY - 1;

      int pattern = 0;
      if (dx >= ChunkSize.width / 2)
      {
        if (pos.x > this->pos.x)
        {
          pattern |= (1 << shiftX);
          this->pos.x += dx;
        }
        else
          this->pos.x -= dx;

        dx /= 2;
      }

      if (dy >= ChunkSize.height / 2)
      {
        if (pos.y > this->pos.y)
        {
          pattern |= (1 << shiftY);
          this->pos.y += dy;
        }
        else
          this->pos.y -= dy;

        dy /= 2;
      }

      if (dz >= ChunkSize.depth / 2)
      {
        if (pos.z > this->pos.z)
        {
          pattern |= (1 << shiftZ);
          this->pos.z += dz;
        }
        else
          this->pos.z -= dz;

        dz /= 2;
      }

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

    //>>>>>>>>>>>>>>>>>>>>>>>>>
    //>>>>>>>>HeapNode>>>>>>>>>
    //>>>>>>>>>>>>>>>>>>>>>>>>>

    Tree::Leaf *Tree::HeapNode::leaf(const Position pos) const
    {
      HeapNode *node = const_cast<Tree::HeapNode *>(this);
      while (!node->isLeaf())
        node = node->child(node->getIndex(pos));

      return static_cast<Tree::Leaf *>(node);
    }

    bool Tree::HeapNode::contains(const Position pos)
    {
      auto leaf = getOrCreateLeaf(pos);
      return leaf->contains(pos);
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

      values[getIndex(pos)] = true;
    }

    std::string Tree::Leaf::show() const
    {
      std::ostringstream s;
      s << "Leaf { " << position << " }";
      return s.str();
    }

    Tree::Leaf *Tree::HeapNode::getOrCreateLeaf(const Position pos)
    {
      if (isLeaf())
        return static_cast<Tree::Leaf *>(this);

      auto child = getOrCreateChild(pos);
      while (!child->isLeaf())
        child = child->getOrCreateChild(pos);

      return static_cast<Tree::Leaf *>(child);
    }

    //>>>>>>>>>>>>>>>>>>>>>>
    //>>>>>>>>NodeZ>>>>>>>>>
    //>>>>>>>>>>>>>>>>>>>>>>
    Tree::NodeZ::NodeZ(const Position midPoint, const SplitDelta splitDelta)
        : midPoint(midPoint), splitDelta(splitDelta) {}

    Tree::HeapNode *Tree::NodeZ::getOrCreateChild(const Position position)
    {
      auto index = getIndex(position);
      if (children[index])
        return children[index].get();

      SplitDelta delta = splitDelta;

      Position nextMidPoint = midPoint;
      nextMidPoint.z += index == 0 ? -delta.z : delta.z;

      delta.z >>= 1;

      if (delta.z >= ChunkSize.depth)
        children[index] = std::make_unique<NodeZ>(nextMidPoint, delta);
      else // Child
      {
        children[index] = std::make_unique<Leaf>(position);
      }

      return children[index].get();
    }

    inline uint16_t Tree::NodeZ::getIndex(const Position &pos) const
    {
      return pos.z > this->midPoint.z;
    }

    std::string Tree::NodeZ::show() const
    {
      std::ostringstream s;
      s << "NodeZ { midPoint: " << midPoint << " }";
      return s.str();
    }

    //>>>>>>>>>>>>>>>>>>>>>>
    //>>>>>>>>NodeY>>>>>>>>>
    //>>>>>>>>>>>>>>>>>>>>>>
    Tree::NodeY::NodeY(const Position midPoint, const SplitDelta splitDelta)
        : midPoint(midPoint), splitDelta(splitDelta) {}

    Tree::HeapNode *Tree::NodeY::getOrCreateChild(const Position position)
    {
      auto index = getIndex(position);
      if (children[index])
        return children[index].get();

      SplitDelta delta = splitDelta;

      Position nextMidPoint = midPoint;
      nextMidPoint.y += index == 0 ? -delta.y : delta.y;

      delta.y >>= 1;

      if (delta.y >= ChunkSize.height)
        children[index] = std::make_unique<NodeY>(nextMidPoint, delta);
      else // Child
      {
        children[index] = std::make_unique<Leaf>(position);
      }

      return children[index].get();
    }

    inline uint16_t Tree::NodeY::getIndex(const Position &pos) const
    {
      return pos.y > this->midPoint.y;
    }

    std::string Tree::NodeY::show() const
    {
      std::ostringstream s;
      s << "NodeY { midPoint: " << midPoint << " }";
      return s.str();
    }

    //>>>>>>>>>>>>>>>>>>>>>>
    //>>>>>>>>NodeX>>>>>>>>>
    //>>>>>>>>>>>>>>>>>>>>>>
    Tree::NodeX::NodeX(const Position midPoint, const SplitDelta splitDelta)
        : midPoint(midPoint), splitDelta(splitDelta) {}

    Tree::HeapNode *Tree::NodeX::getOrCreateChild(const Position position)
    {
      auto index = getIndex(position);
      if (children[index])
        return children[index].get();

      SplitDelta delta = splitDelta;

      Position nextMidPoint = midPoint;
      nextMidPoint.x += index == 0 ? -delta.x : delta.x;

      delta.x >>= 1;

      if (delta.x >= ChunkSize.width)
        children[index] = std::make_unique<NodeX>(nextMidPoint, delta);
      else // Child
      {
        children[index] = std::make_unique<Leaf>(position);
      }

      return children[index].get();
    }

    inline uint16_t Tree::NodeX::getIndex(const Position &pos) const
    {
      return pos.x > this->midPoint.x;
    }

    std::string Tree::NodeX::show() const
    {
      std::ostringstream s;
      s << "NodeX { midPoint: " << midPoint << " }";
      return s.str();
    }

    //>>>>>>>>>>>>>>>>>>>>>>
    //>>>>>>>>NodeXY>>>>>>>>>
    //>>>>>>>>>>>>>>>>>>>>>>

    Tree::NodeXY::NodeXY(const Position midPoint, const SplitDelta splitDelta)
        : midPoint(midPoint), splitDelta(splitDelta) {}

    inline Tree::HeapNode *Tree::NodeXY::getOrCreateChild(const Position position)
    {
      auto index = getIndex(position);
      if (children[index])
        return children[index].get();

      SplitDelta delta = splitDelta;

      Position nextMidPoint = midPoint;
      nextMidPoint.x += (index >> 1) == 0 ? -delta.x : delta.x;
      nextMidPoint.y += (index & 1) == 0 ? -delta.y : delta.y;

      delta.x >>= 1;
      delta.y >>= 1;

      int split = 0;
      split |= (delta.x >= ChunkSize.width) << 1;
      split |= (delta.y >= ChunkSize.height);

      switch (split)
      {
      case 0:
      {
        children[index] = std::make_unique<Leaf>(position);
        break;
      }
      case 0b1:
        children[index] = std::make_unique<NodeY>(nextMidPoint, delta);
        break;
      case 0b10:
        children[index] = std::make_unique<NodeX>(nextMidPoint, delta);
        break;
      case 0b11:
        children[index] = std::make_unique<NodeXY>(nextMidPoint, delta);
        break;
      }

      return children[index].get();
    }

    inline uint16_t Tree::NodeXY::getIndex(const Position &pos) const
    {
      return ((pos.x > midPoint.x) << 1) + (pos.y > midPoint.y);
    }

    std::string Tree::NodeXY::show() const
    {
      std::ostringstream s;
      s << "NodeXY { midPoint: " << midPoint << " }";
      return s.str();
    }

    //>>>>>>>>>>>>>>>>>>>>>>
    //>>>>>>>>NodeXZ>>>>>>>>>
    //>>>>>>>>>>>>>>>>>>>>>>

    Tree::NodeXZ::NodeXZ(const Position midPoint, const SplitDelta splitDelta)
        : midPoint(midPoint), splitDelta(splitDelta) {}

    inline Tree::HeapNode *Tree::NodeXZ::getOrCreateChild(const Position position)
    {
      auto index = getIndex(position);
      if (children[index])
        return children[index].get();

      SplitDelta delta = splitDelta;

      Position nextMidPoint = midPoint;
      nextMidPoint.x += (index >> 1) == 0 ? -delta.x : delta.x;
      nextMidPoint.z += (index & 1) == 0 ? -delta.z : delta.z;

      delta.x >>= 1;
      delta.z >>= 1;

      int split = 0;
      split |= (delta.x >= ChunkSize.width) << 1;
      split |= (delta.z >= ChunkSize.depth);
      switch (split)
      {
      case 0:
        children[index] = std::make_unique<Leaf>(position);
        break;
      case 0b1:
        children[index] = std::make_unique<NodeZ>(nextMidPoint, delta);
        break;
      case 0b10:
        children[index] = std::make_unique<NodeX>(nextMidPoint, delta);
        break;
      case 0b11:
        children[index] = std::make_unique<NodeXZ>(nextMidPoint, delta);
        break;
      }

      return children[index].get();
    }

    uint16_t Tree::NodeXZ::getIndex(const Position &pos) const
    {
      return ((pos.x > midPoint.x) << 1) + (pos.z > midPoint.z);
    }

    std::string Tree::NodeXZ::show() const
    {
      std::ostringstream s;
      s << "NodeXZ { midPoint: " << midPoint << " }";
      return s.str();
    }

    //>>>>>>>>>>>>>>>>>>>>>>
    //>>>>>>>>NodeYZ>>>>>>>>>
    //>>>>>>>>>>>>>>>>>>>>>>

    Tree::NodeYZ::NodeYZ(const Position midPoint, const SplitDelta splitDelta)
        : midPoint(midPoint), splitDelta(splitDelta) {}

    Tree::HeapNode *Tree::NodeYZ::getOrCreateChild(const Position position)
    {
      auto index = getIndex(position);
      if (children[index])
        return children[index].get();

      SplitDelta delta = splitDelta;

      Position nextMidPoint = midPoint;
      nextMidPoint.y += (index >> 1) == 0 ? -delta.y : delta.y;
      nextMidPoint.z += (index & 1) == 0 ? -delta.z : delta.z;

      delta.y >>= 1;
      delta.z >>= 1;

      int split = 0;
      split |= (delta.y >= ChunkSize.height) << 1;
      split |= (delta.z >= ChunkSize.depth);
      switch (split)
      {
      case 0:
        children[index] = std::make_unique<Leaf>(position);
        break;
      case 0b1:
        children[index] = std::make_unique<NodeZ>(nextMidPoint, delta);
        break;
      case 0b10:
        children[index] = std::make_unique<NodeY>(nextMidPoint, delta);
        break;
      case 0b11:
        children[index] = std::make_unique<NodeYZ>(nextMidPoint, delta);
        break;
      }

      return children[index].get();
    }

    inline uint16_t Tree::NodeYZ::getIndex(const Position &pos) const
    {
      return ((pos.y > midPoint.y) << 1) + (pos.z > midPoint.z);
    }

    std::string Tree::NodeYZ::show() const
    {
      std::ostringstream s;
      s << "NodeYZ { midPoint: " << midPoint << " }";
      return s.str();
    }

    //>>>>>>>>>>>>>>>>>>>>>>>>
    //>>>>>>>>NodeXYZ>>>>>>>>>
    //>>>>>>>>>>>>>>>>>>>>>>>>

    Tree::NodeXYZ::NodeXYZ(const Position midPoint, const SplitDelta splitDelta)
        : midPoint(midPoint), splitDelta(splitDelta) {}

    inline Tree::HeapNode *Tree::NodeXYZ::getOrCreateChild(const Position position)
    {
      auto index = getIndex(position);
      if (children[index])
        return children[index].get();

      SplitDelta delta = splitDelta;

      Position nextMidPoint = midPoint;
      nextMidPoint.x += (index >> 2) == 0 ? -delta.x : delta.x;
      nextMidPoint.y += ((index >> 1) & 1) == 0 ? -delta.y : delta.y;
      nextMidPoint.z += (index & 1) == 0 ? -delta.z : delta.z;

      delta.x >>= 1;
      delta.y >>= 1;
      delta.z >>= 1;

      int split = 0;
      split |= (delta.x >= ChunkSize.width) << 2;
      split |= (delta.y >= ChunkSize.height) << 1;
      split |= (delta.z >= ChunkSize.depth);

      switch (split)
      {
      case 0:
        children[index] = std::make_unique<Leaf>(position);
        break;
      case 0b1:
        children[index] = std::make_unique<NodeZ>(nextMidPoint, delta);
        break;
      case 0b10:
        children[index] = std::make_unique<NodeY>(nextMidPoint, delta);
        break;
      case 0b11:
        children[index] = std::make_unique<NodeYZ>(nextMidPoint, delta);
        break;
      case 0b100:
        children[index] = std::make_unique<NodeX>(nextMidPoint, delta);
        break;
      case 0b101:
        children[index] = std::make_unique<NodeXZ>(nextMidPoint, delta);
        break;
      case 0b110:
        children[index] = std::make_unique<NodeXY>(nextMidPoint, delta);
        break;
      case 0b111:
        children[index] = std::make_unique<NodeXYZ>(nextMidPoint, delta);
        break;
      }

      return children[index].get();
    }

    inline uint16_t Tree::NodeXYZ::getIndex(const Position &pos) const
    {
      return ((pos.x > midPoint.x) << 2) | ((pos.y > midPoint.y) << 1) | (pos.z > midPoint.z);
    }

    std::string Tree::NodeXYZ::show() const
    {
      std::ostringstream s;
      s << "NodeXYZ { midPoint: " << midPoint << " }";
      return s.str();
    }

  } // namespace octree
} // namespace vme
