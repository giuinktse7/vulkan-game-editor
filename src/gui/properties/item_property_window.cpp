#include "item_property_window.h"

#include <QQmlContext>
#include <QQmlEngine>
#include <QQmlProperty>
#include <QWidget>
#include <queue>

#include "../../../vendor/rollbear-visit/visit.hpp"
#include "../../config.h"
#include "../../item_location.h"
#include "../../qt/logging.h"
#include "../draggable_item.h"
#include "../gui_thing_image.h"
#include "../mainwindow.h"

namespace ItemPropertyName
{
    constexpr auto Stackable = "isStackable";
    constexpr auto Fluid = "isFluid";
    constexpr auto Writeable = "isWriteable";
    constexpr auto ThingType = "thingType";
} // namespace ItemPropertyName

namespace ObjectName
{
    constexpr auto CountInput = "item_count_input";
    constexpr auto ActionIdInput = "item_actionid_input";
    constexpr auto UniqueIdInput = "item_uniqueid_input";
    constexpr auto FluidTypeInput = "item_fluid_type_input";
    constexpr auto TextInput = "item_text_input";

    constexpr auto FocusedThingImage = "focused_thing_image";

    constexpr auto ItemContainerArea = "item_container_area";

    constexpr auto CreatureSpawnIntervalInput = "creature_spawn_interval_input";

} // namespace ObjectName

namespace QmlFocusedThingType
{
    constexpr auto None = "none";
    constexpr auto Item = "item";
    constexpr auto Creature = "creature";
} // namespace QmlFocusedThingType

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
    engine()->addImageProvider(QLatin1String("creatureLooktypes"), new CreatureImageProvider);

    setSource(filepath);
    setResizeMode(ResizeMode::SizeRootObjectToView);

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

void ItemPropertyWindow::setFocusedCreatureSpawnInterval(int spawnInterval, bool shouldCommit)
{
    if (!state.holds<FocusedCreature>())
    {
        return;
    }

    Creature *creature = state.focusedAs<FocusedCreature>().creature;

    if (spawnInterval == latestCommittedPropertyValues.spawnInterval)
    {
        // Do not commit if the count is the same as when the item was first focused.
        emit spawnIntervalChanged(creature, spawnInterval, false);
    }
    else
    {
        // Necessary to make sure that the correct "old" spawn interval is stored in MapView history
        if (shouldCommit)
        {
            creature->setSpawnInterval(latestCommittedPropertyValues.spawnInterval);
            latestCommittedPropertyValues.spawnInterval = spawnInterval;
        }

        emit spawnIntervalChanged(creature, spawnInterval, shouldCommit);
    }
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

void ItemPropertyWindow::setPropertyItemText(QString qtText)
{
    if (!state.propertyItem || !state.propertyItem->itemType->isWriteable())
        return;

    auto text = qtText.toStdString();

    std::optional<std::string> itemText = state.propertyItem->text();
    bool changed = (itemText.has_value() && itemText != text) || (!itemText.has_value() && text != "");
    if (!changed)
    {
        return;
    }

    emit textChanged(state.propertyItem, text);
}

void ItemPropertyWindow::setPropertyItemCount(int count, bool shouldCommit)
{
    if (count == 0 || !state.propertyItem)
        return;

    // DEBUG_ASSERT(state.propertyItem != nullptr, "No property item.");

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

    auto itemImage = child(ObjectName::FocusedThingImage);
    itemImage->setProperty("source", getItemPixmapString(*item));

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
    uint8_t fluidType = static_cast<uint8_t>(fluidTypeFromIndex(highlightedIndex));
    emit subtypeChanged(state.propertyItem, fluidType, false);
}

void ItemPropertyWindow::setFluidType(int index)
{
    uint8_t fluidType = static_cast<uint8_t>(fluidTypeFromIndex(index));
    if (state.propertyItem->subtype() != latestCommittedPropertyValues.subtype)
    {
        state.propertyItem->setSubtype(latestCommittedPropertyValues.subtype);
        latestCommittedPropertyValues.subtype = fluidType;
        emit subtypeChanged(state.propertyItem, fluidType, true);
    }
}

void ItemPropertyWindow::setPropertyCreature(Creature *creature)
{
    latestCommittedPropertyValues.spawnInterval = creature->spawnInterval();

    auto image = child(ObjectName::FocusedThingImage);
    image->setProperty("source", getCreaturePixmapString(creature));

    rootObject()->setProperty(ItemPropertyName::ThingType, QmlFocusedThingType::Creature);

    auto spawnIntervalInput = child(ObjectName::FocusedThingImage);

    child(ObjectName::CreatureSpawnIntervalInput)->setProperty("text", creature->spawnInterval());
}

void ItemPropertyWindow::setPropertyItem(Item *item)
{
    rootObject()->setProperty(ItemPropertyName::ThingType, QmlFocusedThingType::Item);

    latestCommittedPropertyValues.subtype = item->count();
    state.propertyItem = item;

    // Update UI
    auto itemtype = item->itemType;

    auto itemImage = child(ObjectName::FocusedThingImage);
    itemImage->setProperty("source", getItemPixmapString(*item));

    // ActionID
    auto actionIdInput = child(ObjectName::ActionIdInput);
    setQmlObjectActive(actionIdInput->parent(), true);
    actionIdInput->setProperty("text", item->actionId());

    // UniqueID
    auto uniqueIdInput = child(ObjectName::UniqueIdInput);
    setQmlObjectActive(uniqueIdInput->parent(), true);
    uniqueIdInput->setProperty("text", item->uniqueId());

    // Count / charges
    rootObject()->setProperty(ItemPropertyName::Stackable, itemtype->stackable);
    if (itemtype->stackable)
    {
        child(ObjectName::CountInput)->setProperty("text", item->count());
    }

    // Fluids
    bool usesFluid = itemtype->isSplash() || itemtype->isFluidContainer();
    rootObject()->setProperty(ItemPropertyName::Fluid, usesFluid);
    if (usesFluid)
    {
        uint8_t index = indexOfFluidType(static_cast<FluidType>(state.propertyItem->subtype()));
        child(ObjectName::FluidTypeInput)->setProperty("currentIndex", index);
    }

    // Text
    rootObject()->setProperty(ItemPropertyName::Writeable, itemtype->isWriteable());
    if (itemtype->isWriteable())
    {
        std::string text = state.propertyItem->text().value_or("");
        child(ObjectName::TextInput)->setProperty("text", QString::fromStdString(text));
    }
    else
    {
        child(ObjectName::TextInput)->setProperty("text", "");
    }
}

void ItemPropertyWindow::setFocused(FocusedGround &&ground)
{

    auto item = ground.item();

    state.setFocused(std::move(ground));
    setPropertyItem(item);

    show();
}

void ItemPropertyWindow::setFocused(FocusedItem &&focusedItem)
{
    auto item = focusedItem.item;

    state.setFocused(std::move(focusedItem));
    setPropertyItem(item);

    show();
}

void ItemPropertyWindow::setFocused(FocusedContainer &&container)
{
    auto item = container.containerItem();

    state.setFocused(std::move(container));
    setPropertyItem(item);

    show();
}

void ItemPropertyWindow::setFocused(FocusedCreature &&focusedCreature)
{
    Creature *creature = focusedCreature.creature;
    state.setFocused(std::move(focusedCreature));

    setPropertyCreature(creature);

    show();
}

void ItemPropertyWindow::focusCreature(Creature *creature, Position &position, MapView &mapView)
{
    // Check if the creature is already focused
    if (state.holds<FocusedCreature>())
    {
        auto &focused = state.focusedAs<FocusedCreature>();

        if (focused.creature == creature)
        {
            return;
        }
    }

    resetFocus();

    setSelectedPosition(position);
    setMapView(mapView);

    setFocused(FocusedCreature(creature));
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
    setCount(1);

    QQuickItem *root = rootObject();
    root->setProperty(ItemPropertyName::Stackable, false);
    root->setProperty(ItemPropertyName::Fluid, false);
    root->setProperty(ItemPropertyName::Writeable, false);
    root->setProperty(ItemPropertyName::ThingType, QmlFocusedThingType::None);

    resetMapView();
}

void ItemPropertyWindow::setCount(uint8_t count)
{
    // VME_LOG_D("ItemPropertyWindow::setCount: " << int(count));
    auto countInput = child(ObjectName::CountInput);
    countInput->setProperty("text", count);
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
    setSource(QUrl::fromLocalFile("../../resources/qml/itemPropertyWindow.qml"));
}

QString ItemPropertyWindow::getItemPixmapString(const Item &item) const
{
    return getItemPixmapString(item.serverId(), item.subtype());
}

QString ItemPropertyWindow::getItemPixmapString(int serverId, int subtype) const
{
    return QString::fromStdString(serverId != -1 ? "image://itemTypes/" + std::to_string(serverId) + ":" + std::to_string(subtype) : "");
}

QString ItemPropertyWindow::getCreaturePixmapString(Creature *creature) const
{
    return QtUtil::getCreatureTypeResourcePath(creature->creatureType);
}

void ItemPropertyWindow::State::setFocused(FocusedGround &&ground)
{
    _focusedThing.emplace<FocusedGround>(std::move(ground));
}

void ItemPropertyWindow::State::setFocused(FocusedItem &&item)
{
    _focusedThing.emplace<FocusedItem>(std::move(item));
}

void ItemPropertyWindow::State::setFocused(FocusedContainer &&container)
{
    _focusedThing.emplace<FocusedContainer>(std::move(container));
}
void ItemPropertyWindow::State::setFocused(FocusedCreature &&focusedCreature)
{
    _focusedThing.emplace<FocusedCreature>(std::move(focusedCreature));
}

void ItemPropertyWindow::State::resetFocused()
{
    _focusedThing = std::monostate{};
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
        fluidTypeInput->setProperty("currentIndex", indexOfFluidType(static_cast<FluidType>(state.propertyItem->subtype())));
    }
    else if (state.holds<FocusedCreature>())
    {
        auto spawnIntervalInput = child(ObjectName::CreatureSpawnIntervalInput);
        int spawnInterval = state.focusedAs<FocusedCreature>().creature->spawnInterval();
        spawnIntervalInput->setProperty("text", spawnInterval);
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

QHash<int, QByteArray> FluidTypeModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[TextRole] = "text";
    roles[SubtypeRole] = "subtype";

    return roles;
}

int FluidTypeModel::rowCount(const QModelIndex &parent) const
{
    return indexOfFluidType(FluidType::Mead);
}

QVariant FluidTypeModel::data(const QModelIndex &modelIndex, int role) const
{
    auto index = modelIndex.row();
    if (index < 0 || index >= rowCount())
        return QVariant();

    if (role == TextRole)
    {
        switch (fluidTypeFromIndex(index))
        {
            case FluidType::None:
                return "None";
            case FluidType::Water:
                return "Water";
            case FluidType::Mana:
                return "Mana";
            case FluidType::Beer:
                return "Beer";
            case FluidType::Oil:
                return "Oil";
            case FluidType::Blood:
                return "Blood";
            case FluidType::Slime:
                return "Slime";
            case FluidType::Mud:
                return "Mud";
            case FluidType::Lemonade:
                return "Lemonade";
            case FluidType::Milk:
                return "Milk";
            case FluidType::Wine:
                return "Wine";
            case FluidType::Life:
                return "Life";
            case FluidType::Urine:
                return "Urine";
            case FluidType::CoconutMilk:
                return "Coconut Milk";
            case FluidType::FruitJuice:
                return "Fruit Juice";
            case FluidType::Ink:
                return "Ink";
            case FluidType::Lava:
                return "Lava";
            case FluidType::Rum:
                return "Rum";
            case FluidType::Swamp:
                return "Swamp";
            case FluidType::Tea:
                return "Tea";
            case FluidType::Mead:
                return "Mead";
            default:
                return "Unknown";
        }
    }
    else if (role == SubtypeRole)
    {
        return static_cast<int>(fluidTypeFromIndex(index));
    }

    return QVariant();
}