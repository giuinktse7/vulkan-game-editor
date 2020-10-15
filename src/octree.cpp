#include "octree.h"

#include "debug.h"

namespace vme
{
  namespace octree
  {
    TraversalState::TraversalState(Cube mapSize)
        : pos(
              std::max(ChunkSize.width, mapSize.width / 2),
              std::max(ChunkSize.height, mapSize.height / 2),
              std::max<Position::value_type>(ChunkSize.depth, mapSize.depth / 2)),
          dx(std::max(ChunkSize.width, mapSize.width / 4)),
          dy(std::max(ChunkSize.height, mapSize.height / 4)),
          dz(std::max<Position::value_type>(ChunkSize.depth, mapSize.depth / 4)) {}

    std::string TraversalState::show() const
    {
      std::ostringstream s;
      s << "(" << pos.x << ", " << pos.y << ", " << pos.z << "), deltas: (" << dx << ", " << dy << ", " << dz << ")";
      return s.str();
    }

    int TraversalState::update(Position pos)
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

    CachedNode::CachedNode(HeapNode *parent) : HeapNode(parent) {}

    uint16_t CachedNode::childOffset(const int pattern) const
    {
      return childCacheOffset + pattern;
    }

    void CachedNode::setChildCacheOffset(size_t offset)
    {
      childCacheOffset = offset;
    }

    bool CachedNode::isCachedNode() const noexcept
    {
      return true;
    }

    //>>>>>>>>>>>>>>>>>>>>>>>>>
    //>>>>>>>>HeapNode>>>>>>>>>
    //>>>>>>>>>>>>>>>>>>>>>>>>>

    Leaf *HeapNode::leaf(const Position pos) const
    {
      HeapNode *node = const_cast<HeapNode *>(this);
      while (node && !node->isLeaf())
        node = node->child(node->getIndex(pos));

      return node ? static_cast<Leaf *>(node) : nullptr;
    }

    bool HeapNode::contains(const Position pos)
    {
      auto leaf = getOrCreateLeaf(pos);
      return leaf->contains(pos);
    }

    //>>>>>>>>>>>>>>>>>>>>>
    //>>>>>>>>Leaf>>>>>>>>>
    //>>>>>>>>>>>>>>>>>>>>>

    Leaf::Leaf(const Position pos, HeapNode *parent)
        : HeapNode(parent), position(pos.x - pos.x % ChunkSize.width, pos.y - pos.y % ChunkSize.height, pos.z - pos.z % ChunkSize.depth) {}

    bool Leaf::contains(const Position pos)
    {
      return values[getIndex(pos)];
    }

    inline uint16_t Leaf::getIndex(const Position &pos) const
    {
      return ((pos.x - position.x) * ChunkSize.height + (pos.y - position.y)) * ChunkSize.depth + (pos.z - position.z);
    }

    bool Leaf::add(const Position pos)
    {
      DEBUG_ASSERT(
          (position.x <= pos.x && pos.x < position.x + ChunkSize.width) &&
              (position.y <= pos.y && pos.y < position.y + ChunkSize.height) &&
              (position.z <= pos.z && pos.z < position.z + ChunkSize.depth),
          "The position does not belong to this chunk.");

      values[getIndex(pos)] = true;

      bool changed = boundingBox.include(pos);
      if (changed)
        parent->updateBoundingBox(boundingBox);

      return changed;
    }

    std::string Leaf::show() const
    {
      std::ostringstream s;
      s << "Leaf { " << position << " }";
      return s.str();
    }

    Leaf *HeapNode::getOrCreateLeaf(const Position pos)
    {
      if (isLeaf())
        return static_cast<Leaf *>(this);

      auto node = getOrCreateChild(pos);
      while (!node->isLeaf())
      {
        VME_LOG_D("getOrCreateLeaf: " << node);
        node = node->getOrCreateChild(pos);
      }

      VME_LOG_D("getOrCreateLeaf (leaf): " << node);

      return static_cast<Leaf *>(node);
    }

    //>>>>>>>>>>>>>>>>>>>>>>
    //>>>>>>>>>>>>>>>>>>>>>>
    //>>>>>>>>NodeZ>>>>>>>>>
    //>>>>>>>>>>>>>>>>>>>>>>
    //>>>>>>>>>>>>>>>>>>>>>>
    NodeZ::NodeZ(const Position midPoint, const SplitDelta splitDelta, HeapNode *parent)
        : BaseNode(parent), midPoint(midPoint), splitDelta(splitDelta)
    {
      DEBUG_ASSERT(!parent->isLeaf(), "A parent cannot be a leaf.");
    }

    HeapNode *NodeZ::getOrCreateChild(const Position position)
    {
      auto index = getIndex(position);
      if (children[index])
        return children[index].get();

      SplitDelta delta = splitDelta;

      Position nextMidPoint = midPoint;
      nextMidPoint.z += index == 0 ? -delta.z : delta.z;

      delta.z >>= 1;

      if (delta.z >= ChunkSize.depth)
        children[index] = std::make_unique<NodeZ>(nextMidPoint, delta, this);
      else // Child
      {
        children[index] = std::make_unique<Leaf>(position, this);
      }

      return children[index].get();
    }

    inline uint16_t NodeZ::getIndex(const Position &pos) const
    {
      return pos.z > this->midPoint.z;
    }

    std::string NodeZ::show() const
    {
      std::ostringstream s;
      s << "NodeZ { midPoint: " << midPoint << " }";
      return s.str();
    }

    //>>>>>>>>>>>>>>>>>>>>>>
    //>>>>>>>>>>>>>>>>>>>>>>
    //>>>>>>>>NodeY>>>>>>>>>
    //>>>>>>>>>>>>>>>>>>>>>>
    //>>>>>>>>>>>>>>>>>>>>>>
    NodeY::NodeY(const Position midPoint, const SplitDelta splitDelta, HeapNode *parent)
        : BaseNode(parent), midPoint(midPoint), splitDelta(splitDelta) {}

    HeapNode *NodeY::getOrCreateChild(const Position position)
    {
      auto index = getIndex(position);
      if (children[index])
        return children[index].get();

      SplitDelta delta = splitDelta;

      Position nextMidPoint = midPoint;
      nextMidPoint.y += index == 0 ? -delta.y : delta.y;

      delta.y >>= 1;

      if (delta.y >= ChunkSize.height)
        children[index] = std::make_unique<NodeY>(nextMidPoint, delta, this);
      else // Child
      {
        children[index] = std::make_unique<Leaf>(position, this);
      }

      return children[index].get();
    }

    inline uint16_t NodeY::getIndex(const Position &pos) const
    {
      return pos.y > this->midPoint.y;
    }

    std::string NodeY::show() const
    {
      std::ostringstream s;
      s << "NodeY { midPoint: " << midPoint << " }";
      return s.str();
    }

    //>>>>>>>>>>>>>>>>>>>>>>
    //>>>>>>>>>>>>>>>>>>>>>>
    //>>>>>>>>NodeX>>>>>>>>>
    //>>>>>>>>>>>>>>>>>>>>>>
    //>>>>>>>>>>>>>>>>>>>>>>
    NodeX::NodeX(const Position midPoint, const SplitDelta splitDelta, HeapNode *parent)
        : BaseNode(parent), midPoint(midPoint), splitDelta(splitDelta) {}

    HeapNode *NodeX::getOrCreateChild(const Position position)
    {
      auto index = getIndex(position);
      if (children[index])
        return children[index].get();

      SplitDelta delta = splitDelta;

      Position nextMidPoint = midPoint;
      nextMidPoint.x += index == 0 ? -delta.x : delta.x;

      delta.x >>= 1;

      if (delta.x >= ChunkSize.width)
        children[index] = std::make_unique<NodeX>(nextMidPoint, delta, this);
      else // Child
      {
        children[index] = std::make_unique<Leaf>(position, this);
      }

      return children[index].get();
    }

    inline uint16_t NodeX::getIndex(const Position &pos) const
    {
      return pos.x > this->midPoint.x;
    }

    std::string NodeX::show() const
    {
      std::ostringstream s;
      s << "NodeX { midPoint: " << midPoint << " }";
      return s.str();
    }

    //>>>>>>>>>>>>>>>>>>>>>>
    //>>>>>>>>>>>>>>>>>>>>>>
    //>>>>>>>>NodeXY>>>>>>>>
    //>>>>>>>>>>>>>>>>>>>>>>
    //>>>>>>>>>>>>>>>>>>>>>>

    NodeXY::NodeXY(const Position midPoint, const SplitDelta splitDelta, HeapNode *parent)
        : BaseNode(parent), midPoint(midPoint), splitDelta(splitDelta) {}

    inline HeapNode *NodeXY::getOrCreateChild(const Position position)
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
        children[index] = std::make_unique<Leaf>(position, this);
        break;
      }
      case 0b1:
        children[index] = std::make_unique<NodeY>(nextMidPoint, delta, this);
        break;
      case 0b10:
        children[index] = std::make_unique<NodeX>(nextMidPoint, delta, this);
        break;
      case 0b11:
        children[index] = std::make_unique<NodeXY>(nextMidPoint, delta, this);
        break;
      }

      return children[index].get();
    }

    inline uint16_t NodeXY::getIndex(const Position &pos) const
    {
      return ((pos.x > midPoint.x) << 1) + (pos.y > midPoint.y);
    }

    std::string NodeXY::show() const
    {
      std::ostringstream s;
      s << "NodeXY { midPoint: " << midPoint << " }";
      return s.str();
    }

    //>>>>>>>>>>>>>>>>>>>>>>
    //>>>>>>>>>>>>>>>>>>>>>>
    //>>>>>>>>NodeXZ>>>>>>>>
    //>>>>>>>>>>>>>>>>>>>>>>
    //>>>>>>>>>>>>>>>>>>>>>>

    NodeXZ::NodeXZ(const Position midPoint, const SplitDelta splitDelta, HeapNode *parent)
        : BaseNode(parent), midPoint(midPoint), splitDelta(splitDelta) {}

    inline HeapNode *NodeXZ::getOrCreateChild(const Position position)
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
        children[index] = std::make_unique<Leaf>(position, this);
        break;
      case 0b1:
        children[index] = std::make_unique<NodeZ>(nextMidPoint, delta, this);
        break;
      case 0b10:
        children[index] = std::make_unique<NodeX>(nextMidPoint, delta, this);
        break;
      case 0b11:
        children[index] = std::make_unique<NodeXZ>(nextMidPoint, delta, this);
        break;
      }

      return children[index].get();
    }

    uint16_t NodeXZ::getIndex(const Position &pos) const
    {
      return ((pos.x > midPoint.x) << 1) + (pos.z > midPoint.z);
    }

    std::string NodeXZ::show() const
    {
      std::ostringstream s;
      s << "NodeXZ { midPoint: " << midPoint << " }";
      return s.str();
    }

    //>>>>>>>>>>>>>>>>>>>>>>
    //>>>>>>>>>>>>>>>>>>>>>>
    //>>>>>>>>NodeYZ>>>>>>>>
    //>>>>>>>>>>>>>>>>>>>>>>
    //>>>>>>>>>>>>>>>>>>>>>>

    NodeYZ::NodeYZ(const Position midPoint, const SplitDelta splitDelta, HeapNode *parent)
        : BaseNode(parent), midPoint(midPoint), splitDelta(splitDelta) {}

    HeapNode *NodeYZ::getOrCreateChild(const Position position)
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
        children[index] = std::make_unique<Leaf>(position, this);
        break;
      case 0b1:
        children[index] = std::make_unique<NodeZ>(nextMidPoint, delta, this);
        break;
      case 0b10:
        children[index] = std::make_unique<NodeY>(nextMidPoint, delta, this);
        break;
      case 0b11:
        children[index] = std::make_unique<NodeYZ>(nextMidPoint, delta, this);
        break;
      }

      return children[index].get();
    }

    inline uint16_t NodeYZ::getIndex(const Position &pos) const
    {
      return ((pos.y > midPoint.y) << 1) + (pos.z > midPoint.z);
    }

    std::string NodeYZ::show() const
    {
      std::ostringstream s;
      s << "NodeYZ { midPoint: " << midPoint << " }";
      return s.str();
    }

    //>>>>>>>>>>>>>>>>>>>>>>>>
    //>>>>>>>>>>>>>>>>>>>>>>>>
    //>>>>>>>>NodeXYZ>>>>>>>>>
    //>>>>>>>>>>>>>>>>>>>>>>>>
    //>>>>>>>>>>>>>>>>>>>>>>>>

    NodeXYZ::NodeXYZ(const Position midPoint, const SplitDelta splitDelta, HeapNode *parent)
        : BaseNode(parent), midPoint(midPoint), splitDelta(splitDelta) {}

    inline HeapNode *NodeXYZ::getOrCreateChild(const Position position)
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
        children[index] = std::make_unique<Leaf>(position, this);
        break;
      case 0b1:
        children[index] = std::make_unique<NodeZ>(nextMidPoint, delta, this);
        break;
      case 0b10:
        children[index] = std::make_unique<NodeY>(nextMidPoint, delta, this);
        break;
      case 0b11:
        children[index] = std::make_unique<NodeYZ>(nextMidPoint, delta, this);
        break;
      case 0b100:
        children[index] = std::make_unique<NodeX>(nextMidPoint, delta, this);
        break;
      case 0b101:
        children[index] = std::make_unique<NodeXZ>(nextMidPoint, delta, this);
        break;
      case 0b110:
        children[index] = std::make_unique<NodeXY>(nextMidPoint, delta, this);
        break;
      case 0b111:
        children[index] = std::make_unique<NodeXYZ>(nextMidPoint, delta, this);
        break;
      }

      return children[index].get();
    }

    inline uint16_t NodeXYZ::getIndex(const Position &pos) const
    {
      return ((pos.x > midPoint.x) << 2) | ((pos.y > midPoint.y) << 1) | (pos.z > midPoint.z);
    }

    std::string NodeXYZ::show() const
    {
      std::ostringstream s;
      s << "NodeXYZ { midPoint: " << midPoint << " }";
      return s.str();
    }

    uint16_t CachedNode::getIndex(const Position &pos) const
    {
      ABORT_PROGRAM("getIndex called on CachedNode.");
      return -1;
    }

    void CachedNode::setParent(HeapNode *parent)
    {
      this->parent = parent;
    }

    bool BoundingBox::contains(const BoundingBox &other) const noexcept
    {
      return _top < other._top && _right > other._right && _bottom > other._bottom && _left < other._left;
    }

    void BoundingBox::reset()
    {
      _top = std::numeric_limits<BoundingBox::value_type>::max();
      _right = std::numeric_limits<BoundingBox::value_type>::min();
      _bottom = std::numeric_limits<BoundingBox::value_type>::min();
      _left = std::numeric_limits<BoundingBox::value_type>::max();
    }

    bool BoundingBox::include(const Position pos)
    {
      BoundingBox old = *this;
      _top = std::min<value_type>(_top, pos.y);
      _right = std::max<value_type>(_right, pos.x);
      _bottom = std::max<value_type>(_bottom, pos.y);
      _left = std::min<value_type>(_left, pos.x);

      return old != *this;
    }

    bool BoundingBox::include(const BoundingBox bbox)
    {
      BoundingBox old = *this;
      _top = std::min<value_type>(_top, bbox._top);
      _right = std::max<value_type>(_right, bbox._right);
      _bottom = std::max<value_type>(_bottom, bbox._bottom);
      _left = std::min<value_type>(_left, bbox._left);

      return old != *this;
    }

  } // namespace octree
} // namespace vme
