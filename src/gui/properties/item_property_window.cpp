#include "item_property_window.h"

#include <QQmlContext>
#include <QQmlEngine>
#include <QQmlProperty>
#include <QWidget>

#include "../../../vendor/rollbear-visit/visit.hpp"
#include "../../item_location.h"
#include "../../qt/logging.h"
#include "../draggable_item.h"
#include "../mainwindow.h"

namespace ObjectName
{
    constexpr auto CountSpinBox = "count_spinbox";
    constexpr auto ActionIdSpinBox = "action_id_spinbox";
    constexpr auto UniqueIdSpinBox = "unique_id_spinbox";

    constexpr auto ItemContainerArea = "item_container_area";
} // namespace ObjectName

ItemPropertyWindow::ItemPropertyWindow(QUrl url, MainWindow *mainWindow)
    : _url(url), mainWindow(mainWindow), _wrapperWidget(nullptr)
{
    VME_LOG_D("ItemPropertyWindow address: " << this);
    installEventFilter(new PropertyWindowEventFilter(this));

    containerTree.onContainerItemDrop<&ItemPropertyWindow::itemDropEvent>(this);
    containerTree.onContainerItemDragStart<&ItemPropertyWindow::startContainerItemDrag>(this);

    QVariantMap properties;
    properties.insert("containers", QVariant::fromValue(&containerTree.containerListModel));

    setInitialProperties(properties);

    qmlRegisterSingletonInstance("Vme.context", 1, 0, "C_PropertyWindow", this);

    engine()->addImageProvider(QLatin1String("itemTypes"), new ItemTypeImageProvider);

    setSource(_url);
    VME_LOG_D("After ItemPropertyWindow::setSource");

    QmlApplicationContext *applicationContext = new QmlApplicationContext();
    engine()->rootContext()->setContextProperty("applicationContext", applicationContext);
}

void ItemPropertyWindow::show()
{
    setQmlObjectActive(rootObject(), true);
}

void ItemPropertyWindow::hide()
{
    setQmlObjectActive(rootObject(), false);
}

void ItemPropertyWindow::initializeProperties()
{
    show();
    std::visit(
        util::overloaded{
            [this](const FocusedItem &focusedItem) {
                auto item = focusedItem.item();
                auto itemtype = item->itemType;

                setContainerVisible(item->isContainer());

                auto countSpinBox = child(ObjectName::CountSpinBox);
                setQmlObjectActive(countSpinBox->parent(), itemtype->stackable);

                if (itemtype->stackable)
                {
                    countSpinBox->setProperty("value", item->count());
                }
            },
            [this](const FocusedGround &item) {
                auto countSpinBox = child(ObjectName::CountSpinBox);
                setQmlObjectActive(countSpinBox->parent(), false);

                setContainerVisible(false);
            },
            [](const auto &arg) {
                ABORT_PROGRAM("This should never happen.");
            }},
        state.focusedItem);
}

bool ItemPropertyWindow::event(QEvent *e)
{
    // if (e->type() != QEvent::UpdateRequest)
    // {
    //   qDebug() << e;
    // }

    return QQuickView::event(e);
}

void ItemPropertyWindow::mouseMoveEvent(QMouseEvent *event)
{
    QQuickView::mouseMoveEvent(event);

    // if (dragOperation)
    // {
    //   dragOperation->mouseMoveEvent(event);
    // }
}

void ItemPropertyWindow::mouseReleaseEvent(QMouseEvent *mouseEvent)
{
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

    FocusedGround ground(position, item);

    state.focusedItem.emplace<FocusedGround>(position, item);
    initializeProperties();
}

void ItemPropertyWindow::setFocusedItemCount(int count, bool shouldCommit)
{
    DEBUG_ASSERT(state.holds<FocusedItem>(), "Only a FocusedItem has a count property.");
    auto &focusedItem = state.focusedAs<FocusedItem>();

    ItemLocation location(state.selectedPosition, static_cast<uint16_t>(focusedItem.tileIndex));

    if (count == focusedItem.latestCommittedCount)
    {
        // Do not commit if the count is the same as when the item was first focused.
        emit countChanged(location, count, false);
    }
    else
    {
        // Necessary to make sure that the correct "old" count is stored in MapView history
        if (shouldCommit)
        {
            focusedItem.item()->setCount(focusedItem.latestCommittedCount);
            focusedItem.latestCommittedCount = count;
        }
        emit countChanged(location, count, shouldCommit);
    }
}

void ItemPropertyWindow::focusItem(Item *item, Position &position, MapView &mapView)
{
    if (item->isGround())
    {
        focusGround(item, position, mapView);
        return;
    }
    else if (state.holds<FocusedItem>())
    {
        auto &focusedItem = state.focusedAs<FocusedItem>();
        if (item == focusedItem.item())
        {
            // The item is already focused, update it
            setSelectedPosition(position);

            auto maybeTileIndex = mapView.getTile(position)->indexOf(item);
            DEBUG_ASSERT(maybeTileIndex.has_value(), "The tile did not have the item.");

            focusedItem.tileIndex = static_cast<uint16_t>(maybeTileIndex.value());
            focusedItem.latestCommittedCount = item->count();

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
        // if (containerTree.rootItem() == item)
        // {
        //     // This is already the focused item.
        //     return;
        // }

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

        // if (container->empty())
        // {
        //     std::vector<uint32_t> serverIds{{1987, 2148, 5710, 2673, 2463, 2649}};

        //     for (const auto id : serverIds)
        //         container->addItem(Item(id));
        // }

        containerTree.setRootContainer(&mapView, position, tileIndex, item);
    }

    state.focusedItem.emplace<FocusedItem>(item, tileIndex);
    state.focusedAs<FocusedItem>().latestCommittedCount = item->count();
    initializeProperties();
}

void ItemPropertyWindow::setQmlObjectActive(QObject *qmlObject, bool enabled)
{
    qmlObject->setProperty("visible", enabled);
    qmlObject->setProperty("enabled", enabled);
}

void ItemPropertyWindow::resetFocus()
{
    state.focusedItem = std::monostate{};

    resetSelectedPosition();
    resetMapView();

    containerTree.clear();
    hide();
    setContainerVisible(false);
    setCount(1);

    resetMapView();
}

void ItemPropertyWindow::setCount(uint8_t count)
{
    auto countSpinBox = child(ObjectName::CountSpinBox);
    countSpinBox->setProperty("value", count);
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
}

bool ItemPropertyWindow::itemDropEvent(PropertiesUI::ContainerNode *containerNode, int index, const ItemDrag::DraggableItem *droppedItem)
{
    using DragSource = ItemDrag::DraggableItem::Type;
    auto &focusedItem = state.focusedAs<FocusedItem>();
    if (droppedItem->item() == focusedItem.item())
    {
        VME_LOG_D("Can not add item to itself.");
        return false;
    }

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

            ContainerLocation to(
                state.selectedPosition,
                static_cast<uint16_t>(focusedItem.tileIndex),
                containerNode->indexChain(index));

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

            auto targetContainer = containerNode->container();
            bool movedWithinSameContainer = dropped->container() == targetContainer;

            // Dropped on the same container slot that the drag started
            if (dropped->container() == targetContainer && index == dropped->containerIndices.back())
            {
                containerNode->draggedIndex.reset();
                return true;
            }

            ContainerLocation from(
                dropped->position,
                dropped->tileIndex,
                dropped->containerIndices);

            ContainerLocation to(
                state.selectedPosition,
                static_cast<uint16_t>(focusedItem.tileIndex),
                containerNode->indexChain(index));

            mapView->history.beginTransaction(TransactionType::MoveItems);
            mapView->moveFromContainerToContainer(from, to);
            mapView->history.endTransaction(TransactionType::MoveItems);

            // Update child indices
            if (movedWithinSameContainer)
            {
                containerNode->itemMoved(dropped->containerIndices.back(), index);
                containerNode->draggedIndex.reset();
            }
            else
            {

                // containerNode->itemInserted(index);
            }

            // containerNode->model()->refresh();
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

    const auto &focusedItem = state.focusedAs<FocusedItem>();

    ItemDrag::ContainerItemDrag itemDrag;
    itemDrag.mapView = state.mapView;
    itemDrag.position = state.selectedPosition;

    itemDrag.containerIndices = treeNode->indexChain(index);
    itemDrag.tileIndex = static_cast<uint16_t>(focusedItem.tileIndex);
    // Add treeNode in itemdrag. Needed to update container indices.

    treeNode->draggedIndex.emplace(index);

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
        default:
            break;
    }

    return QObject::eventFilter(obj, event);
}