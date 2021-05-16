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
      mapView(mapView) {}

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

void ContainterTree::modelAddedEvent(UIContainerModel *model)
{
    containerListModel.addItemModel(model);
}

void ContainterTree::modelRemovedEvent(UIContainerModel *model)
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
    trackedContainerItem.onAddressChanged<&ContainerNode::trackedItemChanged>(this);
    trackedContainerItem.onContainerChanged<&ContainerNode::trackedContainerChanged>(this);
}

ContainerNode::ContainerNode(Item *containerItem, ContainerNode *parent)
    : trackedContainerItem(containerItem), _signals(parent->_signals)
{
    trackedContainerItem.onAddressChanged<&ContainerNode::trackedItemChanged>(this);
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
        _uiContainerModel->refresh();
    }
}

void PropertiesUI::ContainerTree::Node::setIndexInParent(int index)
{
    auto k = parent->container()->itemAt(index).name();
    VME_LOG_D(k + ", setIndexInParent: " + std::to_string(index));
    indexInParentContainer = index;

    // Items::items.itemAddressChanged(&parent->container()->itemAt(index));
}

void PropertiesUI::ContainerTree::Root::setIndexInParent(int index)
{
    ABORT_PROGRAM("Can not be used on a Root node.");
}

void ContainerNode::itemInserted(int index)
{
    if (openedChildrenNodes.empty())
        return;

    std::vector<int> indices;

    for (const auto &[i, _] : openedChildrenNodes)
    {
        if (i >= index)
        {
            indices.push_back(i);
        }
    }

    for (int i : indices)
    {
        int newIndex = i + 1;

        auto mapNode = openedChildrenNodes.extract(i);
        mapNode.key() = newIndex;
        openedChildrenNodes.insert(std::move(mapNode));

        openedChildrenNodes.at(newIndex)->setIndexInParent(newIndex);
    }
}

void ContainerNode::itemRemoved(int index)
{
    if (openedChildrenNodes.empty())
        return;

    auto found = openedChildrenNodes.find(index);
    if (found != openedChildrenNodes.end())
    {
        openedChildrenNodes.erase(found);
    }

    std::vector<int> indices;

    for (const auto &[i, _] : openedChildrenNodes)
    {
        if (i >= index)
        {
            indices.push_back(i);
        }
    }

    for (int i : indices)
    {
        int newIndex = i - 1;

        auto mapNode = openedChildrenNodes.extract(i);
        mapNode.key() = newIndex;
        openedChildrenNodes.insert(std::move(mapNode));

        openedChildrenNodes.at(newIndex)->setIndexInParent(newIndex);
    }
}

void ContainerNode::itemMoved(int fromIndex, int toIndex)
{
    if (openedChildrenNodes.empty())
        return;

    std::vector<std::pair<int, int>> changes;

    for (const auto &[i, _] : openedChildrenNodes)
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

    std::vector<decltype(ContainerNode::openedChildrenNodes)::node_type> nodeInsertions;

    for (const auto [fromIndex, toIndex] : changes)
    {
        auto mapNode = openedChildrenNodes.extract(fromIndex);
        mapNode.key() = toIndex;

        nodeInsertions.emplace_back(std::move(mapNode));
        // openedChildrenNodes.insert(std::move(mapNode));

        // openedChildrenNodes.at(toIndex)->setIndexInParent(toIndex);
    }

    for (auto &node : nodeInsertions)
    {
        int newIndex = node.key();
        openedChildrenNodes.insert(std::move(node));

        openedChildrenNodes.at(newIndex)->setIndexInParent(newIndex);
    }
}
void ContainerNode::trackedItemChanged(Item *trackedItem)
{
}

void ContainerNode::trackedContainerChanged(ContainerChange change)
{
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

    if (_uiContainerModel)
    {
        _uiContainerModel->refresh();
    }
}

bool PropertiesUI::ContainerTree::Root::isSelfOrParent(Item *item) const
{
    return item == containerItem();
}

bool PropertiesUI::ContainerTree::Node::isSelfOrParent(Item *item) const
{
    return item == containerItem() || parent->isSelfOrParent(item);
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

PropertiesUI::UIContainerModel *ContainerNode::uiContainerModel()
{
    return _uiContainerModel.has_value() ? &(*_uiContainerModel) : nullptr;
}

void ContainerNode::open()
{
    DEBUG_ASSERT(!opened, "Already opened.");

    _uiContainerModel.emplace(this);
    qDebug() << "Open: " << _uiContainerModel->containerName();

    _signals->postOpened.fire(&_uiContainerModel.value());
    opened = true;
}

void ContainerNode::close()
{
    // Get the indices of opened items
    std::vector<int> indices;
    for (const auto &[index, _] : openedChildrenNodes)
    {
        indices.emplace_back(index);
    }

    for (const int index : indices)
    {
        closeChild(index);
    }

    _signals->preClosed.fire(&_uiContainerModel.value());
    _uiContainerModel.reset();
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

void ContainerNode::closeChild(int index)
{
    auto child = openedChildrenNodes.find(index);
    DEBUG_ASSERT(child != openedChildrenNodes.end(), "The child is not opened.");

    child->second->close();
    openedChildrenNodes.erase(index);
}

void ContainerNode::openChild(int index)
{
    DEBUG_ASSERT(openedChildrenNodes.find(index) == openedChildrenNodes.end(), "The child is already opened.");
    auto &child = container()->itemAt(index);

    DEBUG_ASSERT(child.isContainer(), "Must be container.");
    auto node = createChildNode(index);
    openedChildrenNodes.emplace(index, std::move(node));

    openedChildrenNodes.at(index)->open();
}

void ContainerNode::toggleChild(int index)
{
    auto child = openedChildrenNodes.find(index);
    if (child == openedChildrenNodes.end())
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

void ContainerNode::itemLeftClickedEvent(int index)
{
    _signals->itemLeftClicked.fire(this, index);
}

void ContainerNode::itemDropEvent(int index, ItemDrag::DraggableItem *droppedItem)
{
    // TODO Maybe use fire_accumulate here to see if drop was accepted.
    //Drop **should** always be accepted for now, but that might change in the future.

    DEBUG_ASSERT(index >= 0 && index < _uiContainerModel->capacity(), "Invalid index");

    _signals->itemDropped.fire(this, index, droppedItem);
}
