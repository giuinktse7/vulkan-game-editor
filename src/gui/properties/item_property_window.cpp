#include "item_property_window.h"

#include <QQmlContext>
#include <QQmlEngine>
#include <QQmlProperty>
#include <QWidget>
#include <queue>

#include "../../../vendor/rollbear-visit/visit.hpp"
#include "../../item_location.h"
#include "../../qt/logging.h"
#include "../draggable_item.h"
#include "../mainwindow.h"

const std::pair<std::string, uint8_t> FluidTypeModel::fluidTypes[] = {
    {"None", 0},
    {"Water", 1},
    {"Blood", 2},
    {"Beer", 3},
    {"Slime", 4},
    {"Lemonade", 5},
    {"Milk", 6},
    {"Mana fluid", 7},
    {"Water 2", 9},
    {"Life fluid", 10},
    {"Oil", 11},
    {"Slime 2", 12},
    {"Urine", 13},
    {"Coconut Milk", 14},
    {"Wine", 15},
    {"Mud", 19},
    {"Fruit Juice", 21},
    {"Lava", 26},
    {"Rum", 27},
    {"Swamp", 28},
};

const int FluidTypeModel::_size = sizeof(FluidTypeModel::fluidTypes) / sizeof(FluidTypeModel::fluidTypes[0]);

namespace ObjectName
{
    constexpr auto CountInput = "item_count_input";
    constexpr auto ActionIdInput = "item_actionid_input";
    constexpr auto UniqueIdInput = "item_uniqueid_input";
    constexpr auto FluidTypeInput = "fluid_type_input";

    constexpr auto PropertyItemImage = "property_item_image";

    constexpr auto ItemContainerArea = "item_container_area";
} // namespace ObjectName

ItemPropertyWindow::ItemPropertyWindow(QUrl filepath, MainWindow *mainWindow)
    : _filepath(filepath), mainWindow(mainWindow), _wrapperWidget(nullptr)
{
    installEventFilter(new PropertyWindowEventFilter(this));

    containerTree.onItemLeftClicked<&ItemPropertyWindow::containerItemSelectedEvent>(this);
    containerTree.onContainerItemDrop<&ItemPropertyWindow::itemDropEvent>(this);
    containerTree.onContainerItemDragStart<&ItemPropertyWindow::startContainerItemDrag>(this);

    QVariantMap properties;
    properties.insert("containers", QVariant::fromValue(&containerTree.containerListModel));
    properties.insert("fluidTypeModel", QVariant::fromValue(&fluidTypeModel));

    setInitialProperties(properties);

    qmlRegisterSingletonInstance("Vme.context", 1, 0, "C_PropertyWindow", this);

    engine()->addImageProvider(QLatin1String("itemTypes"), new ItemTypeImageProvider);

    setSource(filepath);

    QmlApplicationContext *applicationContext = new QmlApplicationContext();
    engine()->rootContext()->setContextProperty("applicationContext", applicationContext);

    latestCommittedPropertyValues.actionId = 100;
    latestCommittedPropertyValues.uniqueId = 100;

    latestCommittedPropertyValues.subtype = 1;
}

void ItemPropertyWindow::show()
{
    setQmlObjectActive(rootObject(), true);
}

void ItemPropertyWindow::hide()
{
    setQmlObjectActive(rootObject(), false);
}

bool ItemPropertyWindow::event(QEvent *e)
{
    return QQuickView::event(e);
}

void ItemPropertyWindow::mouseMoveEvent(QMouseEvent *event)
{
    // VME_LOG_D("ItemPropertyWindow::mouseMoveEvent");
    QQuickView::mouseMoveEvent(event);

    // if (dragOperation)
    // {
    //   dragOperation->mouseMoveEvent(event);
    // }
}

void ItemPropertyWindow::mouseReleaseEvent(QMouseEvent *mouseEvent)
{
    // VME_LOG_D("ItemPropertyWindow::mouseReleaseEvent");
    QQuickView::mouseReleaseEvent(mouseEvent);

    if (dragOperation)
    {
        bool accepted = dragOperation->sendDropEvent(mouseEvent);
        VME_LOG_D("Drop accepted? " << accepted);
        if (accepted)
        {
            refresh();
        }

        dragOperation.reset();
    }
}

void ItemPropertyWindow::setMapView(MapView &mapView)
{
    state.mapView = &mapView;
}

void ItemPropertyWindow::setSelectedPosition(const Position &pos)
{
    state.selectedPosition = pos;
}

void ItemPropertyWindow::resetSelectedPosition()
{
    state.selectedPosition = PositionConstants::Zero;
}

void ItemPropertyWindow::resetMapView()
{
    state.mapView = nullptr;
}

void ItemPropertyWindow::focusGround(Item *item, Position &position, MapView &mapView)
{
    if (state.holds<FocusedGround>())
    {
        auto &focusedGround = state.focusedAs<FocusedGround>();
        if (focusedGround.item() == item)
        {
            // Ground already focused
            return;
        }
    }

    resetFocus();

    setMapView(mapView);

    DEBUG_ASSERT(item != nullptr, "Can not focus nullptr ground.");
    DEBUG_ASSERT(mapView.getTile(position)->ground() == item, "This should never happen.");

    setFocused(FocusedGround(position, item));
    setPropertyItem(item);
}

void ItemPropertyWindow::setPropertyItemActionId(int actionId, bool shouldCommit)
{
    DEBUG_ASSERT(state.propertyItem != nullptr, "No property item.");

    Item *item = state.propertyItem;

    if (actionId == latestCommittedPropertyValues.actionId)
    {
        // Do not commit if the count is the same as when the item was first focused.
        emit actionIdChanged(item, actionId, false);
    }
    else
    {
        // Necessary to make sure that the correct "old" count is stored in MapView history
        if (shouldCommit)
        {
            item->setActionId(latestCommittedPropertyValues.actionId);
            latestCommittedPropertyValues.actionId = actionId;
        }
        emit actionIdChanged(item, actionId, shouldCommit);
    }
}

void ItemPropertyWindow::setPropertyItemCount(int count, bool shouldCommit)
{
    DEBUG_ASSERT(state.propertyItem != nullptr, "No property item.");

    Item *item = state.propertyItem;

    if (count == latestCommittedPropertyValues.subtype)
    {
        // Do not commit if the count is the same as when the item was first focused.
        emit subtypeChanged(item, count, false);
    }
    else
    {
        // Necessary to make sure that the correct "old" count is stored in MapView history
        if (shouldCommit)
        {
            item->setCount(latestCommittedPropertyValues.subtype);
            latestCommittedPropertyValues.subtype = count;
        }
        emit subtypeChanged(item, count, shouldCommit);
    }

    // Refresh the container UI if necessary
    if (state.holds<FocusedContainer>())
    {
        std::queue<PropertiesUI::ContainerNode *> nodeQueue;
        nodeQueue.emplace(&(*containerTree.root));
        while (!nodeQueue.empty())
        {
            auto node = nodeQueue.front();
            nodeQueue.pop();

            auto index = node->container()->indexOf(item);
            if (index.has_value())
            {
                node->uiContainerModel()->indexChanged(*index);
                return;
            }

            for (const auto &[_, b] : node->openedChildrenNodes)
            {
                nodeQueue.emplace(b.get());
            }
        }
    }
}

void ItemPropertyWindow::fluidTypeHighlighted(int highlightedIndex)
{
    uint8_t fluidType = FluidTypeModel::fluidTypeFromIndex(highlightedIndex);
    emit subtypeChanged(state.propertyItem, fluidType, false);
}

void ItemPropertyWindow::setFluidType(int index)
{
    uint8_t fluidType = FluidTypeModel::fluidTypeFromIndex(index);
    if (state.propertyItem->subtype() != latestCommittedPropertyValues.subtype)
    {
        state.propertyItem->setSubtype(latestCommittedPropertyValues.subtype);
        latestCommittedPropertyValues.subtype = fluidType;
        emit subtypeChanged(state.propertyItem, fluidType, true);
    }
}

void ItemPropertyWindow::setPropertyItem(Item *item)
{
    latestCommittedPropertyValues.subtype = item->count();
    state.propertyItem = item;

    // Update UI

    auto itemtype = item->itemType;

    auto itemImage = child(ObjectName::PropertyItemImage);
    itemImage->setProperty("source", getItemPixmapString(*item));

    // ActionID
    auto actionIdInput = child(ObjectName::ActionIdInput);
    setQmlObjectActive(actionIdInput->parent(), true);
    actionIdInput->setProperty("value", item->actionId());

    // UniqueID
    auto uniqueIdInput = child(ObjectName::UniqueIdInput);
    setQmlObjectActive(uniqueIdInput->parent(), true);
    uniqueIdInput->setProperty("value", item->uniqueId());

    // Count / charges
    auto countInput = child(ObjectName::CountInput);
    setQmlObjectActive(countInput->parent()->parent(), itemtype->stackable || itemtype->isChargeable());
    if (itemtype->stackable)
    {
        countInput->setProperty("text", item->count());
    }

    // Fluids
    bool usesFluid = itemtype->isSplash() || itemtype->isFluidContainer();

    auto fluidInput = child(ObjectName::FluidTypeInput);
    setQmlObjectActive(fluidInput->parent(), usesFluid);
    if (usesFluid)
    {
        uint8_t index = FluidTypeModel::fluidTypeToIndex(state.propertyItem->subtype());
        fluidInput->setProperty("currentIndex", index);
    }
}

void ItemPropertyWindow::setFocused(FocusedGround &&ground)
{

    auto item = ground.item();

    state.setFocused(std::move(ground));
    setPropertyItem(item);

    setContainerVisible(false);

    show();
}

void ItemPropertyWindow::setFocused(FocusedItem &&focusedItem)
{
    auto item = focusedItem.item;

    state.setFocused(std::move(focusedItem));
    setPropertyItem(item);

    setContainerVisible(false);

    show();
}

void ItemPropertyWindow::setFocused(FocusedContainer &&container)
{
    auto item = container.containerItem();

    state.setFocused(std::move(container));
    setPropertyItem(item);

    setContainerVisible(true);

    show();
}

void ItemPropertyWindow::focusItem(Item *item, Position &position, MapView &mapView)
{
    latestCommittedPropertyValues.subtype = item->count();

    if (item->isGround())
    {
        focusGround(item, position, mapView);
        return;
    }
    else if (state.holds<FocusedItem>())
    {
        auto &focusedItem = state.focusedAs<FocusedItem>();
        if (item == focusedItem.item)
        {
            // The item is already focused, update it
            setSelectedPosition(position);

            auto maybeTileIndex = mapView.getTile(position)->indexOf(item);
            DEBUG_ASSERT(maybeTileIndex.has_value(), "The tile did not have the item.");

            focusedItem.tileIndex = static_cast<uint16_t>(maybeTileIndex.value());

            return;
        }
    }
    else if (state.holds<FocusedContainer>())
    {
        auto &focusedContainer = state.focusedAs<FocusedContainer>();
        if (item == focusedContainer.containerItem())
        {
            setSelectedPosition(position);
            if (state.propertyItem != item)
            {
                setPropertyItem(item);
            }

            auto maybeTileIndex = mapView.getTile(position)->indexOf(item);
            DEBUG_ASSERT(maybeTileIndex.has_value(), "The tile did not have the item.");

            focusedContainer.tileIndex = static_cast<uint16_t>(maybeTileIndex.value());

            return;
        }
    }

    resetFocus();

    setSelectedPosition(position);
    setMapView(mapView);

    auto maybeTileIndex = mapView.getTile(position)->indexOf(item);
    DEBUG_ASSERT(maybeTileIndex.has_value(), "The tile did not have the item.");
    auto tileIndex = static_cast<uint16_t>(maybeTileIndex.value());

    bool isContainer = item->isContainer();

    if (isContainer)
    {
        Container *container = item->getOrCreateContainer();
        container->setParent(&mapView, position);

        // DEBUG CODE, REMOVE
        if (container->empty() && item->serverId() == 2000)
        {
            container->addItem(Item(2595));
            auto &parcel = container->itemAt(0);

            auto bag = Item(1987);
            bag.getOrCreateContainer();

            parcel.getOrCreateContainer()->addItem(std::move(bag));
        }

        containerTree.setRootContainer(&mapView, position, tileIndex, item);

        setFocused(FocusedContainer(item, tileIndex));
    }
    else
    {
        setFocused(FocusedItem(item, tileIndex));
        auto &focusedItem = state.focusedAs<FocusedItem>();
    }
}

void ItemPropertyWindow::setQmlObjectActive(QObject *qmlObject, bool enabled)
{
    if (qmlObject)
    {
        qmlObject->setProperty("visible", enabled);
        qmlObject->setProperty("enabled", enabled);
    }
}

void ItemPropertyWindow::resetFocus()
{
    state.resetFocused();

    resetSelectedPosition();
    resetMapView();
    state.propertyItem = nullptr;

    containerTree.clear();
    hide();
    setContainerVisible(false);
    setCount(1);

    resetMapView();
}

void ItemPropertyWindow::setCount(uint8_t count)
{
    VME_LOG_D("ItemPropertyWindow::setCount: " << int(count));
    auto countInput = child(ObjectName::CountInput);
    countInput->setProperty("value", count);
}

void ItemPropertyWindow::setContainerVisible(bool visible)
{
    auto containerArea = child(ObjectName::ItemContainerArea);
    if (containerArea)
    {
        setQmlObjectActive(containerArea, visible);
    }
    else
    {
        VME_LOG_D("Warning: could not find objectName: " << ObjectName::ItemContainerArea);
    }
}

QWidget *ItemPropertyWindow::wrapInWidget(QWidget *parent)
{
    DEBUG_ASSERT(_wrapperWidget == nullptr, "There is already a wrapper for this window.");

    _wrapperWidget = QWidget::createWindowContainer(this, parent);
    _wrapperWidget->setObjectName("ItemPropertyWindow wrapper");

    return _wrapperWidget;
}

QWidget *ItemPropertyWindow::wrapperWidget() const noexcept
{
    return _wrapperWidget;
}

void ItemPropertyWindow::reloadSource()
{
    VME_LOG_D("ItemPropertyWindow source reloaded.");
    engine()->clearComponentCache();
    setSource(QUrl::fromLocalFile("../resources/qml/itemPropertyWindow.qml"));
}

QString ItemPropertyWindow::getItemPixmapString(const Item &item) const
{
    return getItemPixmapString(item.serverId(), item.subtype());
}

QString ItemPropertyWindow::getItemPixmapString(int serverId, int subtype) const
{
    return QString::fromStdString(serverId != -1 ? "image://itemTypes/" + std::to_string(serverId) + ":" + std::to_string(subtype) : "");
}

void ItemPropertyWindow::State::setFocused(FocusedGround &&ground)
{
    _focusedItem.emplace<FocusedGround>(std::move(ground));
}

void ItemPropertyWindow::State::setFocused(FocusedItem &&item)
{
    _focusedItem.emplace<FocusedItem>(std::move(item));
}

void ItemPropertyWindow::State::setFocused(FocusedContainer &&container)
{
    _focusedItem.emplace<FocusedContainer>(std::move(container));
}

void ItemPropertyWindow::State::resetFocused()
{
    _focusedItem = std::monostate{};
    propertyItem = nullptr;
}

//>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>QML Callbacks>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>

void ItemPropertyWindow::refresh()
{
    if (containerTree.hasRoot())
    {
        auto containerArea = child(ObjectName::ItemContainerArea);
        if (containerArea->property("visible").toBool())
        {
            containerTree.containerListModel.refreshAll();
        }
    }

    if (state.propertyItem != nullptr)
    {
        auto actionIdInput = child(ObjectName::ActionIdInput);
        actionIdInput->setProperty("text", state.propertyItem->actionId());

        auto uniqueIdInput = child(ObjectName::UniqueIdInput);
        uniqueIdInput->setProperty("text", state.propertyItem->uniqueId());

        auto countInput = child(ObjectName::CountInput);
        countInput->setProperty("text", state.propertyItem->count());

        auto fluidTypeInput = child(ObjectName::FluidTypeInput);
        fluidTypeInput->setProperty("currentIndex", FluidTypeModel::fluidTypeToIndex(state.propertyItem->subtype()));
    }
}

bool ItemPropertyWindow::containerItemSelectedEvent(PropertiesUI::ContainerNode *treeNode, int index)
{
    DEBUG_ASSERT(state.holds<FocusedContainer>(), "Must be a focused container.");

    auto &focusedContainer = state.focusedAs<FocusedContainer>();

    auto item = &treeNode->container()->itemAt(index);

    // No action needed
    if (state.propertyItem == item)
    {
        return true;
    }

    setPropertyItem(item);

    return true;
}

bool ItemPropertyWindow::itemDropEvent(PropertiesUI::ContainerNode *targetContainerNode, int dropIndex, const ItemDrag::DraggableItem *droppedItem)
{
    int index = static_cast<int>(std::min(static_cast<size_t>(dropIndex), targetContainerNode->container()->size()));
    index = std::min(index, targetContainerNode->container()->capacity() - 1);

    if (!state.holds<FocusedContainer>())
        return false;

    using DragSource = ItemDrag::DraggableItem::Type;
    auto &focusedItem = state.focusedAs<FocusedContainer>();
    if (droppedItem->item() == focusedItem.containerItem())
    {
        VME_LOG_D("Can not add item to itself.");
        return false;
    }

    if (targetContainerNode->container()->isFull() && !targetContainerNode->container()->hasNonFullContainerAtIndex(index))
    {
        return false;
    }

    int insertionIndex = targetContainerNode->container()->hasNonFullContainerAtIndex(index) ? index : 0;

    MapView *mapView = state.mapView;

    DragSource source = droppedItem->type();
    switch (source)
    {
        case DragSource::MapItem:
        {
            auto dropped = static_cast<const ItemDrag::MapItem *>(droppedItem);

            if (!(mapView == dropped->mapView))
            {
                ABORT_PROGRAM("Drag between different MapViews is not implemented.");
            }

            auto indexChain = targetContainerNode->indexChain(insertionIndex);
            if (targetContainerNode->container()->hasNonFullContainerAtIndex(dropIndex))
            {
                indexChain.emplace_back(0);
            }

            ContainerLocation to(
                state.selectedPosition,
                static_cast<uint16_t>(focusedItem.tileIndex),
                indexChain);

            mapView->history.beginTransaction(TransactionType::MoveItems);
            mapView->moveFromMapToContainer(*dropped->tile, dropped->_item, to);
            mapView->history.endTransaction(TransactionType::MoveItems);
            break;
        }
        case DragSource::ContainerItem:
        {
            auto dropped = static_cast<const ItemDrag::ContainerItemDrag *>(droppedItem);

            if (dropped->mapView != state.mapView)
            {
                ABORT_PROGRAM("Drag between different MapViews is not implemented.");
            }

            auto targetContainer = targetContainerNode->container();
            bool movedWithinSameContainer = dropped->container() == targetContainer;

            // Dropped on the same container slot that the drag started
            if (movedWithinSameContainer && insertionIndex == dropped->containerIndices.back())
            {
                return true;
            }

            ContainerLocation from(
                dropped->position,
                dropped->tileIndex,
                dropped->containerIndices);

            auto indexChain = targetContainerNode->indexChain(insertionIndex);
            if (targetContainerNode->container()->hasNonFullContainerAtIndex(index))
            {
                indexChain.emplace_back(0);
            }

            ContainerLocation to(
                state.selectedPosition,
                static_cast<uint16_t>(focusedItem.tileIndex),
                indexChain);

            mapView->history.beginTransaction(TransactionType::MoveItems);
            mapView->moveFromContainerToContainer(from, to);
            mapView->history.endTransaction(TransactionType::MoveItems);

            VME_LOG_D("Finished move transaction");

            break;
        }
        default:
            VME_LOG_D("[ItemPropertyWindow::itemDropEvent] What do we do here?");
            return false;
    }

    return true;
}

void ItemPropertyWindow::startContainerItemDrag(PropertiesUI::ContainerNode *treeNode, int index)
{
    VME_LOG_D("ItemPropertyWindow::startContainerItemDrag");

    DEBUG_ASSERT(state.holds<FocusedContainer>(), "A drag can only start if a container is focused. Otherwise there is nothing to drag; very likely a bug.");

    const auto &focusedItem = state.focusedAs<FocusedContainer>();

    ItemDrag::ContainerItemDrag itemDrag;
    itemDrag.mapView = state.mapView;
    itemDrag.position = state.selectedPosition;

    itemDrag.containerIndices = treeNode->indexChain(index);
    itemDrag.tileIndex = static_cast<uint16_t>(focusedItem.tileIndex);

    dragOperation.emplace(ItemDrag::DragOperation::create(std::move(itemDrag), state.mapView, this));
    dragOperation->setRenderCondition([this] { return !state.mapView->underMouse(); });
    dragOperation->start();
    dragOperation->onDragFinished<&PropertiesUI::ContainerNode::onDragFinished>(treeNode);
}

QPixmap ItemTypeImageProvider::requestPixmap(const QString &id, QSize *size, const QSize &requestedSize)
{
    //id is either 'serverId' or 'serverId:subtype'.
    auto parts = id.split(':');

    uint32_t serverId;
    uint8_t subtype = 0;

    bool success = false;

    // No subtype if only one part
    if (parts.size() == 1)
    {
        serverId = parts.at(0).toInt(&success);
    }
    else
    {
        DEBUG_ASSERT(parts.size() == 2, "Must have 2 parts here; a serverId and a subtype");
        bool ok;
        serverId = parts.at(0).toInt(&ok);
        if (ok)
        {
            int parsedSubtype = parts.at(1).toInt(&success);
            if (success)
            {
                DEBUG_ASSERT(0 <= subtype && subtype <= UINT8_MAX, "Subtype out of bounds.");
                subtype = static_cast<uint8_t>(parsedSubtype);
            }
        }
    }

    if (!success)
    {
        QPixmap pixmap(32, 32);
        pixmap.fill(QColor("black").rgba());
        return pixmap;
    }

    return QtUtil::itemPixmap(serverId, subtype);
}

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>PropertyWindowEventFilter>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

PropertyWindowEventFilter::PropertyWindowEventFilter(ItemPropertyWindow *parent)
    : QtUtil::EventFilter(static_cast<QObject *>(parent)), propertyWindow(parent) {}

bool PropertyWindowEventFilter::eventFilter(QObject *obj, QEvent *event)
{
    switch (event->type())
    {
        case QEvent::MouseMove:
            if (propertyWindow->dragOperation)
            {
                auto mouseEvent = static_cast<QMouseEvent *>(event);
                propertyWindow->dragOperation->mouseMoveEvent(mouseEvent);
                return false;
            }
            break;
        case QEvent::MouseButtonRelease:
            if (propertyWindow->dragOperation)
            {
                auto mouseEvent = static_cast<QMouseEvent *>(event);

                bool accepted = propertyWindow->dragOperation->sendDropEvent(mouseEvent);
                if (accepted)
                {
                    propertyWindow->refresh();
                }

                propertyWindow->dragOperation.reset();
            }
        default:
            break;
    }

    return QObject::eventFilter(obj, event);
}

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>FluidTypeModel>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

FluidTypeModel::FluidTypeModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

uint8_t FluidTypeModel::fluidTypeFromIndex(int index)
{
    return fluidTypes[index].second;
}

int FluidTypeModel::fluidTypeToIndex(uint8_t fluidType)
{
    for (int i = 0; i < _size; ++i)
    {
        if (fluidTypes[i].second == fluidType)
        {
            return i;
        }
    }

    return -1;
}

QHash<int, QByteArray> FluidTypeModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[TextRole] = "text";
    roles[SubtypeRole] = "subtype";

    return roles;
}

int FluidTypeModel::rowCount(const QModelIndex &parent) const
{
    return _size;
}

QVariant FluidTypeModel::data(const QModelIndex &modelIndex, int role) const
{
    auto index = modelIndex.row();
    if (index < 0 || index >= rowCount())
        return QVariant();

    if (role == TextRole)
    {
        return QString::fromStdString(fluidTypes[index].first);
    }
    else if (role == SubtypeRole)
    {
        return FluidTypeModel::fluidTypeFromIndex(index);
    }

    return QVariant();
}