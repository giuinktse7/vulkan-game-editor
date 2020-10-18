#include "octree.h"

#include "debug.h"

namespace vme
{
  namespace octree
  {
    TraversalState::TraversalState(Cube mapSize, CacheInitInfo cacheInfo)
        : pos(
              std::max(ChunkSize.width, mapSize.width / 2),
              std::max(ChunkSize.height, mapSize.height / 2),
              std::max<Position::value_type>(ChunkSize.depth, mapSize.depth / 2)),
          dx(std::max(ChunkSize.width, mapSize.width / 4)),
          dy(std::max(ChunkSize.height, mapSize.height / 4)),
          dz(std::max<Position::value_type>(ChunkSize.depth, mapSize.depth / 4)),
          cacheInfo(cacheInfo) {}

    std::string TraversalState::show() const
    {
      std::ostringstream s;
      s << "(" << pos.x << ", " << pos.y << ", " << pos.z << "), deltas: (" << dx << ", " << dy << ", " << dz << ")";
      return s.str();
    }

    int TraversalState::update(uint16_t cacheIndex, Position pos)
    {
      int index = 0;
      int shift = 0;

      if (cacheInfo.endZIndex == -1 || cacheIndex < cacheInfo.endZIndex)
      {
        if (pos.z >= this->pos.z)
        {
          this->pos.z += dz;
          index = 1;
        }
        else
        {
          this->pos.z -= dz;
        }

        dz /= 2;
        ++shift;
      }

      if (cacheInfo.endYIndex == -1 || cacheIndex < cacheInfo.endYIndex)
      {
        if (pos.y >= this->pos.y)
        {
          this->pos.y += dy;
          index |= (1 << shift);
        }
        else
        {
          this->pos.y -= dy;
        }

        dy /= 2;
        ++shift;
      }

      if (cacheInfo.endXIndex == -1 || cacheIndex < cacheInfo.endXIndex)
      {
        if (pos.x >= this->pos.x)
        {
          this->pos.x += dx;
          index |= (1 << shift);
        }
        else
        {
          this->pos.x -= dx;
        }

        dx /= 2;
      }

      return index;
    }

    int TraversalState::update2(Position pos)
    {
      int shiftX = (dz >= ChunkSize.depth / 2) + (dy >= ChunkSize.height / 2);
      int shiftY = shiftX - 1;
      int shiftZ = shiftY - 1;

      int pattern = 0;
      if (dx >= ChunkSize.width / 2)
      {
        if (pos.x >= this->pos.x)
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
        if (pos.y >= this->pos.y)
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
        if (pos.z >= this->pos.z)
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

    bool Leaf::addToBoundingBox(const Position pos)
    {
      int x = pos.x % ChunkSize.width;
      int y = pos.y % ChunkSize.height;
      int z = pos.z % ChunkSize.depth;

      bool bboxChange = ((x < low.x || x > high.x) ||
                         (y < low.y || y > high.y) ||
                         (z < low.z || z > high.z)) &&
                        (xs[x] == 0 || ys[y] == 0 || zs[z] == 0);

      xs[x] += 1;
      ys[y] += 1;
      zs[z] += 1;

      if (!bboxChange)
        return false;

      if (count == 1)
      {
        low = {x, y, z};
        high = low;
      }
      else
      {
        low = {
            std::min(x, low.x),
            std::min(y, low.y),
            std::min(z, low.z),
        };

        high = {
            std::max(x, high.x),
            std::max(y, high.y),
            std::max(z, high.z),
        };
      }

      boundingBox = BoundingBox(position.y + low.y, position.x + high.x, position.y + high.y, position.x + low.x);
      return true;
    }

    bool Leaf::removeFromBoundingBox(const Position pos)
    {
      uint16_t x = pos.x % ChunkSize.width;
      uint16_t y = pos.y % ChunkSize.height;
      uint16_t z = pos.z % ChunkSize.depth;

      xs[x] -= 1;
      ys[y] -= 1;
      zs[z] -= 1;

      if (count == 0)
      {
        boundingBox = {};
        low = {};
        high = {};
        return true;
      }

      bool bboxChange = ((x == low.x || x == high.x) ||
                         (y == low.y || y == high.y) ||
                         (z == low.z || z == high.z)) &&
                        (xs[x] == 0 || ys[y] == 0 || zs[z] == 0);
      if (!bboxChange)
        return false;

      // Low
      {
        uint16_t xIndex = low.x;
        for (; xIndex < ChunkSize.width && xs[xIndex] == 0; ++xIndex)
          ;
        low.x = xIndex;

        uint16_t yIndex = low.y;
        for (; yIndex >= 0 && ys[yIndex] == 0; --yIndex)
          ;
        low.y = yIndex;

        uint16_t zIndex = low.z;
        for (; zIndex >= 0 && zs[zIndex] == 0; --zIndex)
          ;
        low.z = zIndex;
      }

      // High
      {
        uint16_t xIndex = high.x;
        for (; xIndex >= 0 && xs[xIndex] == 0; --xIndex)
          ;
        high.x = xIndex;

        uint16_t yIndex = high.y;
        for (; yIndex >= 0 && ys[yIndex] == 0; --yIndex)
          ;
        high.y = yIndex;

        uint16_t zIndex = high.z;
        for (; zIndex >= 0 && zs[zIndex] == 0; --zIndex)
          ;
        high.z = zIndex;
      }

      boundingBox = BoundingBox(position.y + low.y, position.x + high.x, position.y + high.y, position.x + low.x);
      return true;
    }

    bool Leaf::add(const Position pos)
    {
      DEBUG_ASSERT(
          (position.x <= pos.x && pos.x < position.x + ChunkSize.width) &&
              (position.y <= pos.y && pos.y < position.y + ChunkSize.height) &&
              (position.z <= pos.z && pos.z < position.z + ChunkSize.depth),
          "The position does not belong to this chunk.");

      auto index = getIndex(pos);

      if (values[index])
        return false;

      ++count;
      values[index] = true;
      bool bboxChanged = addToBoundingBox(pos);

      if (bboxChanged && !parent->isCachedNode())
        parent->updateBoundingBox(boundingBox);

      return bboxChanged;
    }

    bool Leaf::remove(const Position pos)
    {
      DEBUG_ASSERT(
          (position.x <= pos.x && pos.x < position.x + ChunkSize.width) &&
              (position.y <= pos.y && pos.y < position.y + ChunkSize.height) &&
              (position.z <= pos.z && pos.z < position.z + ChunkSize.depth),
          "The position does not belong to this chunk.");

      auto index = getIndex(pos);
      if (!values[index])
        return false;

      --count;
      values[getIndex(pos)] = false;
      bool bboxChanged = removeFromBoundingBox(pos);
      if (bboxChanged && !parent->isCachedNode())
        parent->updateBoundingBox(boundingBox);

      return bboxChanged;
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
        // VME_LOG_D("getOrCreateLeaf: " << node);
        node = node->getOrCreateChild(pos);
      }

      // VME_LOG_D("getOrCreateLeaf (leaf): " << node);

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

      if (splitDelta.z < ChunkSize.depth)
      {
        children[index] = std::make_unique<Leaf>(position, this);
        return children[index].get();
      }

      Position nextMidPoint = midPoint;
      nextMidPoint.z += index == 0 ? -splitDelta.z : splitDelta.z;

      SplitDelta childDelta = splitDelta;
      childDelta.z >>= 1;

      children[index] = std::make_unique<NodeZ>(nextMidPoint, childDelta, this);
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

      if (splitDelta.y < ChunkSize.height)
      {
        children[index] = std::make_unique<Leaf>(position, this);
        return children[index].get();
      }

      Position nextMidPoint = midPoint;
      nextMidPoint.y += index == 0 ? -splitDelta.y : splitDelta.y;

      SplitDelta childDelta = splitDelta;
      childDelta.y >>= 1;

      children[index] = std::make_unique<NodeY>(nextMidPoint, childDelta, this);
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

      if (splitDelta.x < ChunkSize.width)
      {
        children[index] = std::make_unique<Leaf>(position, this);
        return children[index].get();
      }

      Position nextMidPoint = midPoint;
      nextMidPoint.x += index == 0 ? -splitDelta.x : splitDelta.x;

      SplitDelta childDelta = splitDelta;
      childDelta.x >>= 1;

      children[index] = std::make_unique<NodeX>(nextMidPoint, childDelta, this);
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

      int split = 0;
      split |= (splitDelta.x >= ChunkSize.width) << 1;
      split |= (splitDelta.y >= ChunkSize.height);

      // Child
      if (split == 0)
      {
        children[index] = std::make_unique<Leaf>(position, this);
        return children[index].get();
      }

      Position nextMidPoint = midPoint;
      nextMidPoint.x += (index >> 1) == 0 ? -splitDelta.x : splitDelta.x;
      nextMidPoint.y += (index & 1) == 0 ? -splitDelta.y : splitDelta.y;

      SplitDelta childDelta = splitDelta;
      childDelta.x >>= 1;
      childDelta.y >>= 1;

      switch (split)
      {
      case 0b1:
        children[index] = std::make_unique<NodeY>(std::move(nextMidPoint), childDelta, this);
        break;
      case 0b10:
        children[index] = std::make_unique<NodeX>(std::move(nextMidPoint), childDelta, this);
        break;
      case 0b11:
      {
        children[index] = std::make_unique<NodeXY>(std::move(nextMidPoint), childDelta, this);
        break;
      }

      default:
        ABORT_PROGRAM("Should never happen.");
      }

      return children[index].get();
    }

    inline uint16_t NodeXY::getIndex(const Position &pos) const
    {
      return ((pos.x >= midPoint.x) << 1) + (pos.y >= midPoint.y);
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

      int split = 0;
      split |= (splitDelta.x >= ChunkSize.width) << 1;
      split |= (splitDelta.z >= ChunkSize.depth);

      // Child
      if (split == 0)
      {
        children[index] = std::make_unique<Leaf>(position, this);
        return children[index].get();
      }

      Position nextMidPoint = midPoint;
      nextMidPoint.x += (index >> 1) == 0 ? -splitDelta.x : splitDelta.x;
      nextMidPoint.z += (index & 1) == 0 ? -splitDelta.z : splitDelta.z;

      SplitDelta childDelta = splitDelta;
      childDelta.x >>= 1;
      childDelta.z >>= 1;

      switch (split)
      {
      case 0:
        children[index] = std::make_unique<Leaf>(position, this);
        break;
      case 0b1:
        children[index] = std::make_unique<NodeZ>(nextMidPoint, childDelta, this);
        break;
      case 0b10:
        children[index] = std::make_unique<NodeX>(nextMidPoint, childDelta, this);
        break;
      case 0b11:
        children[index] = std::make_unique<NodeXZ>(nextMidPoint, childDelta, this);
        break;
      default:
        ABORT_PROGRAM("Should never happen.");
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

      int split = 0;
      split |= (splitDelta.y >= ChunkSize.height) << 1;
      split |= (splitDelta.z >= ChunkSize.depth);

      // Child
      if (split == 0)
      {
        children[index] = std::make_unique<Leaf>(position, this);
        return children[index].get();
      }

      Position nextMidPoint = midPoint;
      nextMidPoint.y += (index >> 1) == 0 ? -splitDelta.y : splitDelta.y;
      nextMidPoint.z += (index & 1) == 0 ? -splitDelta.z : splitDelta.z;

      SplitDelta childDelta = splitDelta;
      childDelta.y >>= 1;
      childDelta.z >>= 1;

      switch (split)
      {
      case 0:
        children[index] = std::make_unique<Leaf>(position, this);
        break;
      case 0b1:
        children[index] = std::make_unique<NodeZ>(nextMidPoint, childDelta, this);
        break;
      case 0b10:
        children[index] = std::make_unique<NodeY>(nextMidPoint, childDelta, this);
        break;
      case 0b11:
        children[index] = std::make_unique<NodeYZ>(nextMidPoint, childDelta, this);
        break;
      default:
        ABORT_PROGRAM("Should never happen.");
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

      int split = 0;
      split |= (splitDelta.x >= ChunkSize.width) << 2;
      split |= (splitDelta.y >= ChunkSize.height) << 1;
      split |= (splitDelta.z >= ChunkSize.depth);

      // Child
      if (split == 0)
      {
        children[index] = std::make_unique<Leaf>(position, this);
        return children[index].get();
      }

      Position nextMidPoint = midPoint;
      nextMidPoint.x += (index >> 2) == 0 ? -splitDelta.x : splitDelta.x;
      nextMidPoint.y += ((index >> 1) & 1) == 0 ? -splitDelta.y : splitDelta.y;
      nextMidPoint.z += (index & 1) == 0 ? -splitDelta.z : splitDelta.z;

      SplitDelta childDelta = splitDelta;
      childDelta.x >>= 1;
      childDelta.y >>= 1;
      childDelta.z >>= 1;

      switch (split)
      {
      case 0:
        children[index] = std::make_unique<Leaf>(position, this);
        break;
      case 0b1:
        children[index] = std::make_unique<NodeZ>(nextMidPoint, childDelta, this);
        break;
      case 0b10:
        children[index] = std::make_unique<NodeY>(nextMidPoint, childDelta, this);
        break;
      case 0b11:
        children[index] = std::make_unique<NodeYZ>(nextMidPoint, childDelta, this);
        break;
      case 0b100:
        children[index] = std::make_unique<NodeX>(nextMidPoint, childDelta, this);
        break;
      case 0b101:
        children[index] = std::make_unique<NodeXZ>(nextMidPoint, childDelta, this);
        break;
      case 0b110:
        children[index] = std::make_unique<NodeXY>(nextMidPoint, childDelta, this);
        break;
      case 0b111:
        children[index] = std::make_unique<NodeXYZ>(nextMidPoint, childDelta, this);
        break;
      default:
        ABORT_PROGRAM("Should never happen.");
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
