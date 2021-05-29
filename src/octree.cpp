#include "octree.h"

#include "debug.h"
#include "time_point.h"

namespace vme
{
    namespace octree
    {
        TraversalState::TraversalState(vme::MapSize mapSize, CacheInitInfo cacheInfo)
            : pos(
                  std::max<int>(ChunkSize.width, mapSize.width() / 2),
                  std::max<int>(ChunkSize.height, mapSize.height() / 2),
                  std::max<Position::value_type>(ChunkSize.depth, mapSize.depth() / 2)),
              dx(std::max<int>(ChunkSize.width, mapSize.width() / 4)),
              dy(std::max<int>(ChunkSize.height, mapSize.height() / 4)),
              dz(std::max<Position::value_type>(ChunkSize.depth, mapSize.depth() / 4)),
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

        //>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
        //>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
        //>>>>>>>>>Tree Implementation>>>>>>>
        //>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
        //>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

        Tree Tree::create(const MapSize mapSize)
        {
            vme::octree::CacheInitInfo info = vme::octree::computeCacheStuff(mapSize);

            return Tree(mapSize, info);
        }

        Tree::Tree(vme::MapSize mapSize, CacheInitInfo cacheInfo)
            : width(mapSize.width()), height(mapSize.height()), floors(mapSize.depth()), initialState(mapSize, cacheInfo), cacheInfo(cacheInfo), cachedNodes(cacheInfo.amountToInitialize), cachedHeapNodes(static_cast<size_t>(cacheInfo.count) - cacheInfo.amountToInitialize + 8)
        {
            initializeCache();
        }

        void Tree::initializeCache()
        {
            root.childCacheOffset = 0;

            int exponent = 0;
            if (cacheInfo.endXIndex != 0)
                ++exponent;
            if (cacheInfo.endYIndex != 0)
                ++exponent;
            if (cacheInfo.endZIndex != 0)
                ++exponent;

            int offset = util::power(2, exponent);

            root._childCount = offset;

            // Set parent for root children
            for (int i = 0; i < offset; ++i)
                cachedNodes[i].setParent(&root);

            int cursor = offset;
            for (int i = 0; i < cacheInfo.amountToInitialize; ++i)
            {
                cachedNodes[i].cacheIndex = i;
                cachedNodes[i].childCacheOffset = cursor;

                if (cursor == cacheInfo.endXIndex)
                    offset /= 2;
                if (cursor == cacheInfo.endYIndex)
                    offset /= 2;
                if (cursor == cacheInfo.endZIndex)
                    offset /= 2;

                auto lastChild = std::min<uint16_t>(cursor + offset, cacheInfo.amountToInitialize);
                cachedNodes[i]._childCount = offset;

                for (int k = cursor; k < lastChild; ++k)
                    cachedNodes[k].setParent(&cachedNodes[i]);

                cursor += offset;
            }
        }

        bool Tree::clear()
        {
            bool changed = _size != 0;
            for (const auto i : usedHeapCacheIndices)
                cachedHeapNodes.at(i)->clear();

            for (const auto i : usedCacheIndices)
                cachedNodes.at(i).clear();

            usedCacheIndices.clear();
            usedHeapCacheIndices.clear();

            _size = 0;

            return changed;
        }

        const CachedNode *Tree::getCachedNode(const Position position) const
        {
            TraversalState state = initialState;
            const CachedNode *cached = &root;

            int childIndex;
            uint16_t cacheIndex;
            while (true)
            {
                childIndex = state.update(cached->childCacheOffset, position);
                cacheIndex = cached->childOffset(childIndex);

                if (cacheIndex >= cachedNodes.size())
                    break;

                cached = &cachedNodes[cacheIndex];
            }

            return cached;
        }

        std::optional<std::pair<CachedNode *, HeapNode *>> Tree::fromCache(const Position position) const
        {
            TraversalState state = initialState;
            const CachedNode *cached = &root;

            int childIndex;
            uint16_t cacheIndex;
            while (true)
            {
                childIndex = state.update(cached->childCacheOffset, position);
                cacheIndex = cached->childOffset(childIndex);

                if (cacheIndex >= cachedNodes.size())
                    break;

                cached = &cachedNodes[cacheIndex];
            }

            uint16_t cacheHeapIndex = cacheIndex - cacheInfo.amountToInitialize;
            auto &result = cachedHeapNodes.at(cacheHeapIndex);

            return result ? std::optional<std::pair<CachedNode *, HeapNode *>>{{const_cast<CachedNode *>(cached), result.get()}}
                          : std::nullopt;
        }

        std::pair<CachedNode *, HeapNode *> Tree::getOrCreateFromCache(const Position position)
        {

            // VME_LOG_D("getOrCreateFromCache: " << position);
            TraversalState state = initialState;
            CachedNode *cached = &root;
            // auto k = this;

            // auto test = [position](TraversalState &state) {
            //   VME_LOG_D(": " << (state.pos.x < position.x) << " " << (state.pos.y < position.y) << " " << (state.pos.z < position.z));
            // };

            // VME_LOG_D("Start: " << state.pos);

            int childIndex;
            uint16_t cacheIndex;
            while (true)
            {
                // test(state);
                childIndex = state.update(cached->childCacheOffset, position);
                cacheIndex = cached->childOffset(childIndex);
                // VME_LOG_D("cache: " << cacheIndex << ", child: " << childIndex << " (" << state.pos << ")");

                if (cacheIndex >= cachedNodes.size())
                    break;

                cached = &cachedNodes[cacheIndex];
            }

            uint16_t cacheHeapIndex = cacheIndex - cacheInfo.amountToInitialize;

            usedCacheIndices.emplace_back(cached->cacheIndex);
            usedHeapCacheIndices.emplace_back(cacheHeapIndex);

            // VME_LOG_D("heap: " << cacheHeapIndex);

            int nodeType = 0;
            if (state.dx >= ChunkSize.width / 2)
                nodeType |= (1 << 2);
            if (state.dy >= ChunkSize.height / 2)
                nodeType |= (1 << 1);
            if (state.dz >= ChunkSize.depth / 2)
                nodeType |= (1 << 0);

            // For leaf node
            if (nodeType == 0)
                state.pos = position;

            if (!cachedHeapNodes.at(cacheHeapIndex))
                cachedHeapNodes.at(cacheHeapIndex) = heapNodeFromSplitPattern(
                    nodeType,
                    state.pos,
                    SplitDelta{state.dx, state.dy, state.dz},
                    cached);

            return {cached, cachedHeapNodes.at(cacheHeapIndex).get()};
        }

        bool Tree::contains(const Position pos) const
        {
            auto l = getLeaf(pos);
            return l && l->contains(pos);
        }

        bool Tree::add(const Position pos)
        {
            // VME_LOG_D("Size before add: " << size());
            auto [cached, leaf] = getOrCreateLeaf(pos);

            const auto [changed, bboxChanged] = leaf->add(pos);
            if (changed)
            {
                ++_size;

                if (_size == 1)
                    _single = pos;
                else if (_size == 2)
                    _single.reset();
            }

            if (bboxChanged)
                cached->updateBoundingBoxCached(*this);

            DEBUG_ASSERT(boundingBox().has_value(), "Boundingbox empty after add");

            // VME_LOG_D("Size after add: " << size());
            return changed;
        }

        bool Tree::remove(const Position pos)
        {
            // VME_LOG_D("Size before remove: " << size());

            auto [cached, leaf] = getOrCreateLeaf(pos);

            const auto [changed, bboxChanged] = leaf->remove(pos);

            if (changed)
            {
                --_size;

                if (_size == 1)
                {
                    // the findOnlyPosition() call takes ~8 microseconds on my machine (Release mode). Could
                    // cause lag if this happens many times in a single frame (e.g. if
                    // an item is moved many times in a single transaction)
                    // Performance could be improved for this case by caching two positions
                    // p1 and p2 instead of one (_single). Then, when going from 2 -> 1 items
                    // simply set p1 to whichever of p1 and p2 that was not removed.
                    _single = findOnlyPosition();
                }
                else if (_size == 0)
                    _single.reset();
            }

            if (bboxChanged)
                cached->updateBoundingBoxCached(*this);

            // VME_LOG_D("Size after remove: " << size());
            return changed;
        }

        Position Tree::findOnlyPosition() const
        {
            DEBUG_ASSERT(_size == 1, "Impossible to find >only< position if there is more than one position in the tree.");

            if (_single.has_value())
                return *_single;

            VME_LOG_D("Had to use findOnlyPosition()");

            auto res = *begin();
            return res;
        }

        std::optional<Position> Tree::onlyPosition() const
        {
            return _size == 1 ? _single : std::nullopt;
        }

        Leaf *Tree::getLeaf(const Position position) const
        {
            if (mostRecentLeaf.second && mostRecentLeaf.second->encloses(position))
                return mostRecentLeaf.second;

            auto maybeCached = fromCache(position);
            if (!maybeCached)
                return nullptr;
            const auto &[cached, node] = maybeCached.value();
            auto leaf = node->leaf(position);
            markAsRecent(cached, leaf);
            return leaf;
        }

        std::pair<CachedNode *, Leaf *> Tree::getOrCreateLeaf(const Position pos)
        {
            if (mostRecentLeaf.second && mostRecentLeaf.second->encloses(pos))
            {
                return mostRecentLeaf;
            }

            const auto [cached, node] = getOrCreateFromCache(pos);
            auto leaf = node->getOrCreateLeaf(pos);

            markAsRecent(cached, leaf);

            return {cached, leaf};
        }

        const std::optional<BoundingBox> Tree::boundingBox() const noexcept
        {
            return root.boundingBox();
        }

        void Tree::markAsRecent(CachedNode *cached, Leaf *leaf) const
        {
            mostRecentLeaf = {cached, leaf};
        }

        HeapNode *Tree::getChild(int index, const HeapNode *node) const
        {
            if (!node->isCachedNode())
                return node->child(index);

            // For cached nodes
            const CachedNode *cached = static_cast<const CachedNode *>(node);
            auto childIndex = cached->childOffset(index);

            if (cached->childCacheOffset < cacheInfo.amountToInitialize)
            {
                return &cachedNodes.at(childIndex);
            }
            else
            {
                auto &child = cachedHeapNodes.at(childIndex - cacheInfo.amountToInitialize);
                return child ? child.get() : nullptr;
            }
        }

        //>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
        //>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
        //>>>>End of Tree Implementation>>>>>
        //>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
        //>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

        CachedNode::CachedNode(HeapNode *parent)
            : HeapNode(parent) {}

        int CachedNode::childCount() const noexcept
        {
            return _childCount;
        }

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

        void CachedNode::updateBoundingBoxCached(const Tree &tree)
        {
            // VME_LOG_D("updateBoundingBoxCached before: " << boundingBox);
            _boundingBox = {};

            int splits = 0;
            if (cacheIndex < tree.cacheInfo.endXIndex)
                ++splits;
            if (cacheIndex < tree.cacheInfo.endYIndex)
                ++splits;
            if (cacheIndex < tree.cacheInfo.endZIndex)
                ++splits;

            DEBUG_ASSERT(splits != 0, "Update bounding box for leaf node? Bug?");

            int amountOfChildren = util::power(2, splits);
            for (int i = 0; i < amountOfChildren; ++i)
            {
                HeapNode *node = nullptr;
                if (childCacheOffset >= tree.cacheInfo.amountToInitialize) // Get from HeapNode cache
                {
                    auto &ptr = tree.cachedHeapNodes.at(static_cast<size_t>(childCacheOffset) + i - tree.cacheInfo.amountToInitialize);
                    if (ptr)
                    {
                        node = ptr.get();
                    }
                }
                else // Get from CachedNode cache
                {
                    node = &tree.cachedNodes.at(static_cast<size_t>(childCacheOffset) + i);
                }

                if (node)
                {
                    auto &box = node->boundingBox();
                    if (box)
                        addBoundingBox(box.value());
                }
            }

            // VME_LOG_D("updateBoundingBoxCached after: " << boundingBox);
            if (parent)
                static_cast<CachedNode *>(parent)->updateBoundingBoxCached(tree);
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

        bool HeapNode::contains(const Position pos) const
        {
            auto l = leaf(pos);
            return l && l->contains(pos);
        }

        //>>>>>>>>>>>>>>>>>>>>>
        //>>>>>>>>Leaf>>>>>>>>>
        //>>>>>>>>>>>>>>>>>>>>>

        Leaf::Leaf(const Position pos, HeapNode *parent)
            : HeapNode(parent), position(pos.x - pos.x % ChunkSize.width, pos.y - pos.y % ChunkSize.height, static_cast<int8_t>(pos.z - pos.z % ChunkSize.depth))
        {
        }

        bool Leaf::contains(const Position pos) const
        {
            uint16_t index = getIndex(pos);
            if (index >= values.size())
                return false;

            return values[index];
        }

        bool Leaf::encloses(const Position pos) const
        {
            return (position.x <= pos.x && pos.x < position.x + ChunkSize.width) &&
                   (position.y <= pos.y && pos.y < position.y + ChunkSize.height) &&
                   (position.z <= pos.z && pos.z < position.z + ChunkSize.depth);
        }

        inline uint16_t Leaf::getIndex(const Position &pos) const
        {
            return ((pos.z - position.z) * ChunkSize.height + (pos.y - position.y)) * ChunkSize.width + (pos.x - position.x);
            //return ((pos.x - position.x) * ChunkSize.height + (pos.y - position.y)) * ChunkSize.depth + (pos.z - position.z);
        }

        bool Leaf::addToBoundingBox(const Position pos)
        {
            int x = pos.x % ChunkSize.width;
            int y = pos.y % ChunkSize.height;
            int z = pos.z % ChunkSize.depth;

            // Must be before changing the xs, ys, zs values.
            auto a = std::holds_alternative<BoundingBox>(_boundingBox);
            auto b = (x < low.x || x > high.x) ||
                     (y < low.y || y > high.y) ||
                     (z < low.z || z > high.z);
            auto c = (xs[x] == 0 || ys[y] == 0 || zs[z] == 0);
            bool bboxChange = !a || (b && c);

            xs[x] += 1;
            ys[y] += 1;
            zs[z] += 1;

#ifdef _DEBUG_VME
            {
                size_t total = 0;
                for (int i = 0; i < ChunkSize.width; ++i)
                    total += xs[i];
                for (int i = 0; i < ChunkSize.height; ++i)
                    total += ys[i];
                for (int i = 0; i < ChunkSize.depth; ++i)
                    total += zs[i];

                DEBUG_ASSERT((total / 3) == _count, "Total indices count must be equal to _count.");
            }
#endif

            // std::ostringstream s;
            // int t = 0;
            // s << "Count: " << _count << "( ";
            // {
            //   int total = 0;
            //   for (int i = 0; i < ChunkSize.width; ++i)
            //     total += xs[i];
            //   s << "x: " << total << ", ";
            //   t += total;
            // }
            // {
            //   int total = 0;
            //   for (int i = 0; i < ChunkSize.height; ++i)
            //     total += ys[i];
            //   s << "y: " << total << ", ";
            //   t += total;
            // }
            // {
            //   int total = 0;
            //   for (int i = 0; i < ChunkSize.depth; ++i)
            //     total += zs[i];
            //   s << "z: " << total << ")";
            //   t += total;
            // }
            // s << ", diff: " << (int)_count - t / 3;
            // VME_LOG_D(s.str());

            if (!bboxChange)
            {
                return false;
            }

            if (_count == 1)
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

            Position from(position.x + low.x, position.y + low.y, position.z + low.z);
            Position to(position.x + high.x, position.y + high.y, position.z + high.z);

            _boundingBox = BoundingBox(from, to);

            return true;
        } // namespace octree

        bool Leaf::removeFromBoundingBox(const Position pos)
        {
            uint16_t x = pos.x % ChunkSize.width;
            uint16_t y = pos.y % ChunkSize.height;
            uint16_t z = pos.z % ChunkSize.depth;

            xs[x] -= 1;
            ys[y] -= 1;
            zs[z] -= 1;

            if (_count == 0)
            {
                _boundingBox = {};
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
                if (x == low.x)
                {
                    uint16_t xIndex = low.x;
                    for (; xIndex < ChunkSize.width && xs[xIndex] == 0; ++xIndex)
                        ;
                    low.x = xIndex;
                }

                if (y == low.y)
                {
                    uint16_t yIndex = low.y;
                    for (; yIndex >= 0 && ys[yIndex] == 0; ++yIndex)
                        ;
                    low.y = yIndex;
                }

                if (z == low.z)
                {
                    uint16_t zIndex = low.z;
                    for (; zIndex >= 0 && zs[zIndex] == 0; ++zIndex)
                        ;
                    low.z = zIndex;
                }
            }

            // High
            {
                if (x == high.x)
                {
                    uint16_t xIndex = high.x;
                    for (; xIndex >= 0 && xs[xIndex] == 0; --xIndex)
                        ;
                    high.x = xIndex;
                }

                if (y == high.y)
                {
                    uint16_t yIndex = high.y;
                    for (; yIndex >= 0 && ys[yIndex] == 0; --yIndex)
                        ;
                    high.y = yIndex;
                }

                if (z == high.z)
                {
                    uint16_t zIndex = high.z;
                    for (; zIndex >= 0 && zs[zIndex] == 0; --zIndex)
                        ;
                    high.z = zIndex;
                }
            }

            Position from(position.x + low.x, position.y + low.y, position.z + low.z);
            Position to(position.x + high.x, position.y + high.y, position.z + high.z);

            _boundingBox = BoundingBox(from, to);
            return true;
        }

        Leaf::UpdateResult Leaf::add(const Position pos)
        {
            DEBUG_ASSERT(
                (position.x <= pos.x && pos.x < position.x + ChunkSize.width) &&
                    (position.y <= pos.y && pos.y < position.y + ChunkSize.height) &&
                    (position.z <= pos.z && pos.z < position.z + ChunkSize.depth),
                "The position does not belong to this chunk.");

            auto index = getIndex(pos);

            if (values[index])
                return {false, false};

            ++_count;
            values[index] = true;
            bool bboxChanged = addToBoundingBox(pos);

            if (bboxChanged && !parent->isCachedNode())
                parent->updateBoundingBox();

            return {true, bboxChanged};
        }

        Leaf::UpdateResult Leaf::remove(const Position pos)
        {
            DEBUG_ASSERT(
                (position.x <= pos.x && pos.x < position.x + ChunkSize.width) &&
                    (position.y <= pos.y && pos.y < position.y + ChunkSize.height) &&
                    (position.z <= pos.z && pos.z < position.z + ChunkSize.depth),
                "The position does not belong to this chunk.");

            auto index = getIndex(pos);
            if (!values[index])
                return {false, false};

            --_count;
            values[getIndex(pos)] = false;
            bool bboxChanged = removeFromBoundingBox(pos);
            if (bboxChanged && !parent->isCachedNode())
                parent->updateBoundingBox();

            return {true, bboxChanged};
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
            return ((pos.x >= midPoint.x) << 2) | ((pos.y >= midPoint.y) << 1) | (pos.z >= midPoint.z);
        }

        std::string NodeXYZ::show() const
        {
            std::ostringstream s;
            s << "NodeXYZ { midPoint: " << midPoint << " }";
            return s.str();
        }

        bool CachedNode::addBoundingBox(BoundingBox bbox)
        {
            if (!std::holds_alternative<BoundingBox>(_boundingBox))
            {
                _boundingBox = bbox;
                return true;
            }

            BoundingBox &box = std::get<BoundingBox>(_boundingBox);
            return box.include(bbox);
        }

        uint16_t CachedNode::getIndex(const Position &pos) const
        {
            ABORT_PROGRAM("getIndex called on CachedNode.");
            return -1;
        }

        void CachedNode::clear()
        {
            _boundingBox = {};
            if (parent)
                static_cast<CachedNode *>(parent)->clear();
        }

        void CachedNode::setParent(HeapNode *parent)
        {
            this->parent = parent;
        }

        bool BoundingBox::contains(const BoundingBox &other) const noexcept
        {
            return (_min.x <= other._min.x && _min.y <= other._min.y && _min.z <= other._min.z) &&
                   (_max.x >= other._max.x && _max.y >= other._max.y && _max.z >= other._max.z);
        }

        bool BoundingBox::include(const Position pos)
        {
            BoundingBox old = *this;
            _min.x = std::min<value_type>(_min.x, pos.x);
            _min.y = std::min<value_type>(_min.y, pos.y);
            _min.z = std::min<value_type>(_min.z, pos.z);

            _max.x = std::max<value_type>(_max.x, pos.x);
            _max.y = std::max<value_type>(_max.y, pos.y);
            _max.z = std::max<value_type>(_max.z, pos.z);

            return old != *this;
        }

        bool BoundingBox::include(const BoundingBox bbox)
        {
            BoundingBox old = *this;
            _min.x = std::min<value_type>(_min.x, bbox._min.x);
            _min.y = std::min<value_type>(_min.y, bbox._min.y);
            _min.z = std::min<value_type>(_min.z, bbox._min.z);

            _max.x = std::max<value_type>(_max.x, bbox._max.x);
            _max.y = std::max<value_type>(_max.y, bbox._max.y);
            _max.z = std::max<value_type>(_max.z, bbox._max.z);

            return old != *this;
        }

        //>>>>>>>>>>>>>>>>>>>>>>>>>>>>
        //>>>>>>>>>>>>>>>>>>>>>>>>>>>>
        //>>>>Tree::leafIterator>>>>>
        //>>>>>>>>>>>>>>>>>>>>>>>>>>>>
        //>>>>>>>>>>>>>>>>>>>>>>>>>>>>

        Tree::leafIterator::leafIterator()
            : tree(nullptr), value(nullptr) {}
        Tree::leafIterator::leafIterator(const Tree *tree)
            : tree(tree), value(nullptr)
        {
            if (tree->root.empty())
            {
                // VME_LOG_D("Root was empty");
                isEnd = true;
            }
            else
            {
                HeapNode *rootPtr = const_cast<HeapNode *>(static_cast<const HeapNode *>(&tree->root));
                nodes.emplace(rootPtr);
                nextLeaf();
            }
        }

        Tree::leafIterator Tree::leafIterator::end()
        {
            auto it = Tree::leafIterator();
            it.isEnd = true;

            return it;
        }

        Tree::leafIterator &Tree::leafIterator::operator++()
        {
            nextLeaf();
            return *this;
        }

        Tree::leafIterator Tree::leafIterator::operator++(int junk)
        {
            Tree::leafIterator it = *this;
            ++(*this);

            return it;
        }

        bool Tree::leafIterator::operator==(const Tree::leafIterator &rhs) const
        {
            return !(isEnd ^ rhs.isEnd) && ((isEnd && rhs.isEnd) || (value == rhs.value));
        }

        void Tree::leafIterator::nextLeaf()
        {
            const HeapNode *current;
            while (!nodes.empty())
            {
                // VME_LOG_D("??: " << nodes.size());

                current = nodes.front();
                nodes.pop();

                if (!current->empty())
                {
                    if (current->isLeaf())
                    {
                        // VME_LOG_D("Leaf in nextLeaf!");
                        value = static_cast<const Leaf *>(current);
                        return;
                    }
                    else
                    {
                        for (int i = 0; i < current->childCount(); ++i)
                        {
                            auto child = tree->getChild(i, current);
                            if (child && !child->empty())
                                nodes.emplace(child);
                        }
                    }
                }
            }

            // VME_LOG_D("Found no leaf.");

            isEnd = true;
        }

        bool Tree::leafIterator::finished() const noexcept
        {
            return isEnd;
        }

        //>>>>>>>>>>>>>>>>>>>>>>>>>>>>
        //>>>>>>>>>>>>>>>>>>>>>>>>>>>>
        //>>>>>>>Tree::iterator>>>>>>>
        //>>>>>>>>>>>>>>>>>>>>>>>>>>>>
        //>>>>>>>>>>>>>>>>>>>>>>>>>>>>

        Tree::iterator::iterator()
        {
        }
        vme::octree::Tree::iterator::iterator(const vme::octree::Tree *tree)
            : leafIterator(tree)
        {
            if (leafIterator == Tree::leafIterator::end() || !currentLeaf())
            {
                isEnd = true;
                return;
            }

            value = currentLeaf()->min();
            topLeft = value;
            bottomRight = currentLeaf()->max();

            if (!currentLeaf()->contains(value))
                nextValue();
        }

        const Leaf *Tree::iterator::currentLeaf() const
        {
            return *leafIterator;
        }

        void Tree::iterator::nextValue()
        {
            do
            {
                ++value.x;
                if (value.x > bottomRight.x)
                {
                    value.x = topLeft.x;
                    ++value.y;

                    if (value.y > bottomRight.y)
                    {
                        value.y = topLeft.y;
                        ++value.z;

                        if (value.z > bottomRight.z)
                        {
                            ++leafIterator;
                            if (leafIterator.finished())
                            {
                                isEnd = true;
                                return;
                            }

                            topLeft = currentLeaf()->min();
                            bottomRight = currentLeaf()->max();
                            value = topLeft;
                        }
                    }
                }
            } while (!currentLeaf()->contains(value));
        }

        Tree::iterator &Tree::iterator::operator++()
        {
            nextValue();
            return *this;
        }

        Tree::iterator Tree::iterator::operator++(int junk)
        {
            Tree::iterator it = *this;
            ++(*this);

            return it;
        }

        bool Tree::iterator::operator==(const Tree::iterator &rhs) const
        {
            return !(isEnd ^ rhs.isEnd) && ((isEnd && rhs.isEnd) || (value == rhs.value));
        }

        Tree::iterator Tree::iterator::end()
        {
            auto it = Tree::iterator();
            it.isEnd = true;

            return it;
        }

    } // namespace octree
} // namespace vme
