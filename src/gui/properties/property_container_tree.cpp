#include "property_container_tree.h"

#include "../../debug.h"
#include "../../item.h"
#include "../../items.h"

//>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>ContainterTree>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>
using ContainterTree = PropertiesUI::ContainerTree;
using ContainerNode = PropertiesUI::ContainerNode;

PropertiesUI::ContainerTree::ContainerTree()
{
    _signals.postOpened.connect<&ContainerTree::modelAddedEvent>(this);
    _signals.preClosed.connect<&ContainerTree::modelRemovedEvent>(this);
}

PropertiesUI::ContainerTree::Root::Root(
    MapView *mapView,
    Position mapPosition,
    uint16_t tileIndex,
    Item *containerItem,
    ContainerSignals *_signals)
    : ContainerNode(containerItem, _signals),
      mapPosition(mapPosition),
      mapView(mapView),
      tileIndex(tileIndex) {}

Item *ContainerNode::containerItem() const
{
    return trackedContainerItem.item();
}

Container *ContainerNode::container()
{
    return trackedContainerItem.item()->getOrCreateContainer();
}

std::unique_ptr<ContainerNode> PropertiesUI::ContainerTree::Root::createChildNode(int index)
{
    auto childItem = &container()->itemAt(index);

    auto childContainer = childItem->getOrCreateContainer();
    childContainer->setParent(mapView, mapPosition);

    return std::make_unique<ContainerTree::Node>(childItem, this, index);
}

PropertiesUI::ContainerTree::Node::Node(Item *containerItem, ContainerNode *parent, uint16_t parentIndex)
    : ContainerNode(containerItem, parent), parent(parent), indexInParentContainer(parentIndex)
{
    VME_LOG_D("Node() with parent: " << parent);
}

std::unique_ptr<ContainerNode> PropertiesUI::ContainerTree::Node::createChildNode(int index)
{
    auto childItem = &container()->itemAt(index);

    auto childContainer = childItem->getOrCreateContainer();
    childContainer->setParent(parent->container());

    return std::make_unique<ContainerTree::Node>(childItem, this, index);
}

const Item *ContainterTree::rootItem() const
{
    return root ? root->containerItem() : nullptr;
}

bool ContainterTree::hasRoot() const noexcept
{
    return root.has_value();
}

void ContainterTree::setRootContainer(MapView *mapView, Position position, uint16_t tileIndex, Item *containerItem)
{
    root.emplace(mapView, position, tileIndex, containerItem, &_signals);
    root->open();
}

void ContainterTree::clear()
{
    root.reset();
    containerListModel.clear();
}

void ContainterTree::modelAddedEvent(ContainerModel *model)
{
    containerListModel.addItemModel(model);
}

void ContainterTree::modelRemovedEvent(ContainerModel *model)
{
    containerListModel.remove(model);
}

//>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>ContainerNode>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>

ContainerNode::ContainerNode(Item *containerItem, ContainerSignals *_signals)
    : trackedContainerItem(containerItem), _signals(_signals)
{
    trackedContainerItem.onChanged<&ContainerNode::trackedItemChanged>(this);
    trackedContainerItem.onContainerChanged<&ContainerNode::trackedContainerChanged>(this);
}

ContainerNode::ContainerNode(Item *containerItem, ContainerNode *parent)
    : trackedContainerItem(containerItem), _signals(parent->_signals)
{
    trackedContainerItem.onChanged<&ContainerNode::trackedItemChanged>(this);
    trackedContainerItem.onContainerChanged<&ContainerNode::trackedContainerChanged>(this);
}

ContainerNode::~ContainerNode()
{
    if (opened)
    {
        close();
    }
}

void ContainerNode::onDragFinished(ItemDrag::DragOperation::DropResult result)
{
    using DropResult = ItemDrag::DragOperation::DropResult;

    if (result == DropResult::Accepted)
    {
        // TODO
        // It would be faster to only refresh the changed indices. But this should
        // not make a significant difference in performance, because the model will
        // have at most ~25 items (max capacity of the largest container item).
        // _model->refresh();

        // if (draggedIndex.has_value())
        // {
        //     int index = draggedIndex.value();
        //     itemRemoved(index);
        // }
    }
}

void PropertiesUI::ContainerTree::Node::setIndexInParent(int index)
{
    indexInParentContainer = index;
    Items::items.itemMoved(&parent->container()->itemAt(index));
}

void PropertiesUI::ContainerTree::Root::setIndexInParent(int index)
{
    ABORT_PROGRAM("Can not be used on a Root node.");
}

void ContainerNode::itemInserted(int index)
{
    if (children.empty())
        return;

    std::vector<int> indices;

    for (const auto &[i, _] : children)
    {
        if (i >= index)
        {
            indices.push_back(i);
        }
    }

    for (int i : indices)
    {
        int newIndex = i + 1;

        auto mapNode = children.extract(i);
        mapNode.key() = newIndex;
        children.insert(std::move(mapNode));

        children.at(newIndex)->setIndexInParent(newIndex);
    }
}

void ContainerNode::itemRemoved(int index)
{
    if (children.empty())
        return;

    auto found = children.find(index);
    if (found != children.end())
    {
        children.erase(found);
    }

    std::vector<int> indices;

    for (const auto &[i, _] : children)
    {
        if (i >= index)
        {
            indices.push_back(i);
        }
    }

    for (int i : indices)
    {
        int newIndex = i - 1;

        auto mapNode = children.extract(i);
        mapNode.key() = newIndex;
        children.insert(std::move(mapNode));

        children.at(newIndex)->setIndexInParent(newIndex);
    }
}

void ContainerNode::itemMoved(int fromIndex, int toIndex)
{
    if (children.empty())
        return;

    std::vector<std::pair<int, int>> changes;

    for (const auto &[i, _] : children)
    {
        int newIndex;
        if (i == fromIndex)
        {
            newIndex = toIndex;
        }
        else if (fromIndex < i && i <= toIndex)
        {
            newIndex = i - 1;
        }
        else if (toIndex <= i && i < fromIndex)
        {
            newIndex = i + 1;
        }
        else
        {
            continue;
        }

        changes.emplace_back(std::make_pair(i, newIndex));
    }

    for (const auto [fromIndex, toIndex] : changes)
    {
        auto mapNode = children.extract(fromIndex);
        mapNode.key() = toIndex;
        children.insert(std::move(mapNode));

        children.at(toIndex)->setIndexInParent(toIndex);
    }
}
void ContainerNode::trackedItemChanged(Item *trackedItem)
{
    VME_LOG_D("trackedItemChanged for: " << containerItem()->name());
    // for (auto &entry : children)
    // {
    //     auto &i = container()->itemAt(entry.first);
    //     Items::items.itemMoved(&i);
    // }
}

void ContainerNode::trackedContainerChanged(ContainerChange change)
{
    VME_LOG_D("trackedContainerChanged for " << containerItem()->name() << ":" << change);
    switch (change.type)
    {
        case ContainerChangeType::Insert:
            itemInserted(change.index);
            break;
        case ContainerChangeType::Remove:
            itemRemoved(change.index);
            break;
        case ContainerChangeType::MoveInSameContainer:
            itemMoved(change.index, change.toIndex);
            break;
    }

    _model->refresh();
}

std::vector<uint16_t> ContainerNode::indexChain() const
{
    return indexChain(0);
}

std::vector<uint16_t> ContainerNode::indexChain(int index) const
{
    std::vector<uint16_t> result;
    result.emplace_back(index);

    auto current = this;
    while (!current->isRoot())
    {
        auto node = static_cast<const ContainerTree::Node *>(current);
        result.emplace_back(node->indexInParentContainer);
        current = node->parent;
    }

    std::reverse(result.begin(), result.end());
    return result;
}

PropertiesUI::ContainerModel *ContainerNode::model()
{
    return _model.has_value() ? &(*_model) : nullptr;
}

void ContainerNode::open()
{
    DEBUG_ASSERT(!opened, "Already opened.");

    _model.emplace(this);
    _signals->postOpened.fire(&_model.value());
    opened = true;
}

void ContainerNode::close()
{
    _signals->preClosed.fire(&_model.value());
    _model.reset();
    opened = false;
}

void ContainerNode::toggle()
{
    if (opened)
    {
        close();
    }
    else
    {
        open();
    }
}

void ContainerNode::openChild(int index)
{
    DEBUG_ASSERT(children.find(index) == children.end(), "The child is already opened.");
    auto &child = container()->itemAt(index);

    DEBUG_ASSERT(child.isContainer(), "Must be container.");
    auto node = createChildNode(index);
    children.emplace(index, std::move(node));
    auto &c = *children.at(index);
    ContainerTree::Node *d = dynamic_cast<ContainerTree::Node *>(&c);
    children.at(index)->open();
}

void ContainerNode::toggleChild(int index)
{
    auto child = children.find(index);
    if (child == children.end())
    {
        openChild(index);
        return;
    }
    child->second->toggle();
}

void ContainerNode::itemDragStartEvent(int index)
{
    _signals->itemDragStarted.fire(this, index);
}

void ContainerNode::itemDropEvent(int index, ItemDrag::DraggableItem *droppedItem)
{
    // TODO Maybe use fire_accumulate here to see if drop was accepted.
    //Drop **should** always be accepted for now, but that might change in the future.

    // Index must be in [0, size - 1]
    index = std::min(index, std::max(_model->size() - 1, 0));

    _signals->itemDropped.fire(this, index, droppedItem);
}
