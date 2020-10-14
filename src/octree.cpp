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
        auto firstChild = (i + 1) * offset;
        auto lastChild = std::min<uint16_t>(firstChild + offset, CachedNodeCount.amountToInitialize);

        cachedNodes[i].cacheIndex = i;
        cachedNodes[i].childCacheOffset = firstChild;

        for (int k = firstChild; k < lastChild; ++k)
          cachedNodes[k].setParent(&cachedNodes[i]);

        if (i == CachedNodeCount.endXIndex)
          offset /= 2;
        if (i == CachedNodeCount.endYIndex)
          offset /= 2;
        if (i == CachedNodeCount.endZIndex)
          offset /= 2;
      }
    }

    Tree::HeapNode *Tree::fromCache(const Position position) const
    {
      Tree::TraversalState state = top;

      auto cached = root;
      int pattern = state.update(position);
      uint16_t index = cached.childOffset(pattern);

      while (index < CachedNodeCount.amountToInitialize)
      {
        cached = cachedNodes[index];
        index = cached.childOffset(pattern);
      }

      uint16_t cacheHeapIndex = index - CachedNodeCount.amountToInitialize;

      auto &result = cachedHeapNodes.at(cacheHeapIndex);
      return result ? result.get() : nullptr;
    }

    std::pair<Tree::CachedNode *, Tree::HeapNode *> Tree::getOrCreateFromCache(const Position position)
    {
      Tree::TraversalState state = top;

      CachedNode *cached = &root;
      int pattern = state.update(position);
      uint16_t index = cached->childOffset(pattern);

      while (index < CachedNodeCount.amountToInitialize)
      {
        cached = &cachedNodes[index];
        index = cached->childOffset(pattern);
      }
      // VME_LOG_D("Final cache state: " << state.show());
      // VME_LOG_D(index);

      // state.update(pos);
      uint16_t cacheHeapIndex = index - CachedNodeCount.amountToInitialize;

      SplitDelta splitDelta{state.dx, state.dy, state.dz};

      int currentPattern = 0;
      if (state.dx >= ChunkSize.width)
        currentPattern |= (1 << 2);
      if (state.dy >= ChunkSize.height)
        currentPattern |= (1 << 1);
      if (state.dz >= ChunkSize.depth)
        currentPattern |= (1 << 0);

      if (!cachedHeapNodes.at(cacheHeapIndex))
        cachedHeapNodes.at(cacheHeapIndex) = Tree::heapNodeFromSplitPattern(currentPattern, state.pos, splitDelta, cached);

      return {cached, cachedHeapNodes.at(cacheHeapIndex).get()};
    }

    bool Tree::contains(const Position pos) const
    {
      auto l = leaf(pos);
      return l && l->contains(pos);
    }

    void Tree::add(const Position pos)
    {
      auto [cached, node] = getOrCreateFromCache(pos);
      auto l = node->getOrCreateLeaf(pos);
      VME_LOG_D(l->position);

      bool changed = l->add(pos);
      if (changed)
        cached->updateBoundingBoxCached(*this);

      // if (node->isLeaf())
      // {
      //   VME_LOG_D("Cached node was leaf.");
      //   auto leaf = static_cast<Leaf *>(node);
      //   leaf->add(pos);
      //   VME_LOG_D("Leaf node: " << leaf->position);
      // }
      // else
      // {
      //   // TODO test this
      //   VME_LOG_D("Cached node was not leaf.");
      //   auto leaf = node->getOrCreateLeaf(pos);
      //   leaf->add(pos);
      //   VME_LOG_D("Leaf node (non-cached): " << leaf->position);
      // }
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

    Tree::Leaf *Tree::leaf(const Position position) const
    {
      return fromCache(position)->leaf(position);
    }

    Tree::Leaf *Tree::getOrCreateLeaf(const Position position)
    {
      return fromCache(position)->getOrCreateLeaf(position);
    }

    Tree::CachedNode::CachedNode(HeapNode *parent) : HeapNode(parent) {}

    uint16_t Tree::CachedNode::childOffset(const int pattern) const
    {
      return childCacheOffset + pattern;
    }

    void Tree::CachedNode::setChildCacheOffset(size_t offset)
    {
      childCacheOffset = offset;
    }

    bool Tree::CachedNode::isCachedNode() const noexcept
    {
      return true;
    }

    //>>>>>>>>>>>>>>>>>>>>>>>>>
    //>>>>>>>>>>>>>>>>>>>>>>>>>
    //>>>>>>>>END TREE>>>>>>>>>
    //>>>>>>>>>>>>>>>>>>>>>>>>>
    //>>>>>>>>>>>>>>>>>>>>>>>>>

    //>>>>>>>>>>>>>>>>>>>>>>>>>
    //>>>>>>>>HeapNode>>>>>>>>>
    //>>>>>>>>>>>>>>>>>>>>>>>>>

    Tree::Leaf *Tree::HeapNode::leaf(const Position pos) const
    {
      HeapNode *node = const_cast<Tree::HeapNode *>(this);
      while (node && !node->isLeaf())
        node = node->child(node->getIndex(pos));

      return node ? static_cast<Tree::Leaf *>(node) : nullptr;
    }

    bool Tree::HeapNode::contains(const Position pos)
    {
      auto leaf = getOrCreateLeaf(pos);
      return leaf->contains(pos);
    }

    //>>>>>>>>>>>>>>>>>>>>>
    //>>>>>>>>Leaf>>>>>>>>>
    //>>>>>>>>>>>>>>>>>>>>>

    Tree::Leaf::Leaf(const Position pos, Tree::HeapNode *parent)
        : HeapNode(parent), position(pos.x - pos.x % ChunkSize.width, pos.y - pos.y % ChunkSize.height, pos.z - pos.z % ChunkSize.depth) {}

    bool Tree::Leaf::contains(const Position pos)
    {
      return values[getIndex(pos)];
    }

    inline uint16_t Tree::Leaf::getIndex(const Position &pos) const
    {
      return ((pos.x - position.x) * ChunkSize.height + (pos.y - position.y)) * ChunkSize.depth + (pos.z - position.z);
    }

    bool Tree::Leaf::add(const Position pos)
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

      auto node = getOrCreateChild(pos);
      while (!node->isLeaf())
      {
        VME_LOG_D("getOrCreateLeaf: " << node);
        node = node->getOrCreateChild(pos);
      }

      VME_LOG_D("getOrCreateLeaf (leaf): " << node);

      return static_cast<Tree::Leaf *>(node);
    }

    //>>>>>>>>>>>>>>>>>>>>>>
    //>>>>>>>>>>>>>>>>>>>>>>
    //>>>>>>>>NodeZ>>>>>>>>>
    //>>>>>>>>>>>>>>>>>>>>>>
    //>>>>>>>>>>>>>>>>>>>>>>
    Tree::NodeZ::NodeZ(const Position midPoint, const SplitDelta splitDelta, Tree::HeapNode *parent)
        : BaseNode(parent), midPoint(midPoint), splitDelta(splitDelta)
    {
      DEBUG_ASSERT(!parent->isLeaf(), "A parent cannot be a leaf.");
    }

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
        children[index] = std::make_unique<NodeZ>(nextMidPoint, delta, this);
      else // Child
      {
        children[index] = std::make_unique<Leaf>(position, this);
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
    //>>>>>>>>>>>>>>>>>>>>>>
    //>>>>>>>>NodeY>>>>>>>>>
    //>>>>>>>>>>>>>>>>>>>>>>
    //>>>>>>>>>>>>>>>>>>>>>>
    Tree::NodeY::NodeY(const Position midPoint, const SplitDelta splitDelta, Tree::HeapNode *parent)
        : BaseNode(parent), midPoint(midPoint), splitDelta(splitDelta) {}

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
        children[index] = std::make_unique<NodeY>(nextMidPoint, delta, this);
      else // Child
      {
        children[index] = std::make_unique<Leaf>(position, this);
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
    //>>>>>>>>>>>>>>>>>>>>>>
    //>>>>>>>>NodeX>>>>>>>>>
    //>>>>>>>>>>>>>>>>>>>>>>
    //>>>>>>>>>>>>>>>>>>>>>>
    Tree::NodeX::NodeX(const Position midPoint, const SplitDelta splitDelta, Tree::HeapNode *parent)
        : BaseNode(parent), midPoint(midPoint), splitDelta(splitDelta) {}

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
        children[index] = std::make_unique<NodeX>(nextMidPoint, delta, this);
      else // Child
      {
        children[index] = std::make_unique<Leaf>(position, this);
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
    //>>>>>>>>>>>>>>>>>>>>>>
    //>>>>>>>>NodeXY>>>>>>>>
    //>>>>>>>>>>>>>>>>>>>>>>
    //>>>>>>>>>>>>>>>>>>>>>>

    Tree::NodeXY::NodeXY(const Position midPoint, const SplitDelta splitDelta, Tree::HeapNode *parent)
        : BaseNode(parent), midPoint(midPoint), splitDelta(splitDelta) {}

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
    //>>>>>>>>>>>>>>>>>>>>>>
    //>>>>>>>>NodeXZ>>>>>>>>
    //>>>>>>>>>>>>>>>>>>>>>>
    //>>>>>>>>>>>>>>>>>>>>>>

    Tree::NodeXZ::NodeXZ(const Position midPoint, const SplitDelta splitDelta, Tree::HeapNode *parent)
        : BaseNode(parent), midPoint(midPoint), splitDelta(splitDelta) {}

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
    //>>>>>>>>>>>>>>>>>>>>>>
    //>>>>>>>>NodeYZ>>>>>>>>
    //>>>>>>>>>>>>>>>>>>>>>>
    //>>>>>>>>>>>>>>>>>>>>>>

    Tree::NodeYZ::NodeYZ(const Position midPoint, const SplitDelta splitDelta, Tree::HeapNode *parent)
        : BaseNode(parent), midPoint(midPoint), splitDelta(splitDelta) {}

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
    //>>>>>>>>>>>>>>>>>>>>>>>>
    //>>>>>>>>NodeXYZ>>>>>>>>>
    //>>>>>>>>>>>>>>>>>>>>>>>>
    //>>>>>>>>>>>>>>>>>>>>>>>>

    Tree::NodeXYZ::NodeXYZ(const Position midPoint, const SplitDelta splitDelta, Tree::HeapNode *parent)
        : BaseNode(parent), midPoint(midPoint), splitDelta(splitDelta) {}

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

    uint16_t Tree::CachedNode::getIndex(const Position &pos) const
    {
      ABORT_PROGRAM("getIndex called on CachedNode.");
      return -1;
    }

    void Tree::CachedNode::setParent(Tree::HeapNode *parent)
    {
      this->parent = parent;
    }

    void Tree::CachedNode::updateBoundingBoxCached(const Tree &tree)
    {
      VME_LOG_D("updateBoundingBoxCached before: " << boundingBox);
      boundingBox.reset();

      int splits = 0;
      if (cacheIndex < CachedNodeCount.endXIndex)
        ++splits;
      if (cacheIndex < CachedNodeCount.endYIndex)
        ++splits;
      if (cacheIndex < CachedNodeCount.endZIndex)
        ++splits;

      DEBUG_ASSERT(splits != 0, "Update bounding box for leaf node? Bug?");

      int childCount = power(2, splits);
      for (int i = 0; i < childCount; ++i)
      {
        if (childCacheOffset > CachedNodeCount.amountToInitialize) // Get from HeapNode cache
        {
          auto &node = tree.cachedHeapNodes.at(childCacheOffset + i - CachedNodeCount.amountToInitialize);
          if (node)
            boundingBox.include(node->boundingBox);
        }
        else // Get from CachedNode cache
        {
          auto node = tree.cachedNodes.at(childCacheOffset + i);
          boundingBox.include(node.boundingBox);
        }
      }

      VME_LOG_D("updateBoundingBoxCached after: " << boundingBox);
      if (parent)
        static_cast<CachedNode *>(parent)->updateBoundingBoxCached(tree);
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
