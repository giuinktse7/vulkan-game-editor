#include "item_property_window.h"

#include <QQmlContext>
#include <QQmlEngine>
#include <QQmlProperty>
#include <QWidget>

#include "../../vendor/rollbear-visit/visit.hpp"
#include "../qt/logging.h"
#include "draggable_item.h"
#include "mainwindow.h"

namespace ObjectName
{
  constexpr auto CountSpinBox = "count_spinbox";
  constexpr auto ActionIdSpinBox = "action_id_spinbox";
  constexpr auto UniqueIdSpinBox = "unique_id_spinbox";

  constexpr auto ItemContainerArea = "item_container_area";
} // namespace ObjectName

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

ItemPropertyWindow::ItemPropertyWindow(QUrl url, MainWindow *mainWindow)
    : _url(url), mainWindow(mainWindow), _wrapperWidget(nullptr)
{
  VME_LOG_D("ItemPropertyWindow address: " << this);
  installEventFilter(new PropertyWindowEventFilter(this));

  containerModel = std::make_unique<GuiItemContainer::ContainerModel>();

  // itemContainerModels.emplace_back(std::make_unique<GuiItemContainer::ItemModel>());

  QVariantMap properties;
  properties.insert("containers", QVariant::fromValue(containerModel.get()));
  // properties.insert("containerItems", QVariant::fromValue(itemContainerModels.front().get()));

  setInitialProperties(properties);

  qmlRegisterSingletonInstance("Vme.context", 1, 0, "C_PropertyWindow", this);

  // engine()->rootContext()->setContextProperty("contextPropertyWindow", this);
  engine()->addImageProvider(QLatin1String("itemTypes"), new ItemTypeImageProvider);

  setSource(_url);

  QmlApplicationContext *applicationContext = new QmlApplicationContext();
  engine()->rootContext()->setContextProperty("applicationContext", applicationContext);
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

void ItemPropertyWindow::mouseReleaseEvent(QMouseEvent *event)
{
  QQuickView::mouseReleaseEvent(event);

  if (dragOperation)
  {
    bool accepted = dragOperation->sendDropEvent(event);
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

void ItemPropertyWindow::resetMapView()
{
  state.mapView = nullptr;
}

void ItemPropertyWindow::focusGround(Position &position, MapView &mapView)
{
  setMapView(mapView);

  setContainerVisible(false);
  // containerModel.itemContainerModels.front()->reset();
  containerModel->clear();

  setCount(1);

  FocusedGround ground;
  ground.position = position;
  ground.ground = mapView.getTile(position)->ground();

  DEBUG_ASSERT(ground.ground != nullptr, "Can not focus nullptr ground.");

  state.focusedItem = ground;
}

void ItemPropertyWindow::focusItem(Item &item, Position &position, MapView &mapView)
{
  containerModel->clear();

  if (item.isGround())
  {
    focusGround(position, mapView);
    return;
  }

  setMapView(mapView);

  FocusedItem focusedItem;

  auto index = mapView.getTile(position)->indexOf(&item);

  DEBUG_ASSERT(index.has_value(), "The tile did not have the item.");

  bool isContainer = item.isContainer();

  if (isContainer)
  {
    auto container = ContainerItem::wrap(item).value();
    if (container.empty())
    {
      std::vector<uint32_t> serverIds{{1987, 2148, 5710, 2673, 2463, 2649}};

      for (const auto id : serverIds)
        container.addItem(Item(id));
    }

    // itemContainerModels.front()->setContainer(container);
    containerModel->addContainerItem(container);

    // TODO Just for debugging. Should be removed
    if (container.itemAt(0).isContainer())
    {
      containerModel->addContainerItem(ContainerItem::wrap(container.itemAt(0)).value());
    }
  }
  else
  {
    // itemContainerModels.front()->reset();
  }

  setContainerVisible(isContainer);
  setCount(item.count());

  focusedItem.item = &item;
  focusedItem.tileIndex = index.value();
  focusedItem.position = position;

  state.focusedItem = focusedItem;
}

void ItemPropertyWindow::resetFocus()
{
  // itemContainerModels.front()->reset();
  containerModel->clear();
  setContainerVisible(false);
  setCount(1);
  state.focusedItem = std::monostate{};

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
    containerArea->setProperty("visible", visible);
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

bool ItemPropertyWindow::testDropEvent(QByteArray serializedMapItem)
{
  auto mapItem = ItemDrag::DraggableItem::deserialize(serializedMapItem);
  if (!mapItem)
  {
    VME_LOG("[Warning]: Could not read MapItem from qml QByteArray.");
    return false;
  }

  return true;
}

void GuiItemContainer::ItemModel::refresh()
{
  beginResetModel();
  endResetModel();
}

void ItemPropertyWindow::refresh()
{
  // itemContainerModels.front()->refresh();
  containerModel->refresh(0);
}

bool ItemPropertyWindow::itemDropEvent(int index, QByteArray serializedDraggableItem)
{
  VME_LOG_D("Index: " << index);
  auto droppedItem = ItemDrag::DraggableItem::deserialize(serializedDraggableItem);
  if (!droppedItem)
  {
    VME_LOG("[Warning]: Could not read DraggableItem from qml QByteArray.");
    return false;
  }

  // Only accept items that can be picked up
  if (!droppedItem->item()->itemType->hasFlag(AppearanceFlag::Take))
  {
    return false;
  }

  if (!state.holds<FocusedItem>())
  {
    return false;
  }

  auto &focusedItem = state.focusedAs<FocusedItem>();
  if (droppedItem->item() == focusedItem.item)
  {
    VME_LOG_D("Can not add item to itself.");
    return false;
  }

  auto mapView = state.mapView;

  ItemDrag::DraggableItem *droppedItemPtr = droppedItem.get();

  if (util::hasDynamicType<ItemDrag::MapItem *>(droppedItemPtr))
  {
    auto dropped = static_cast<ItemDrag::MapItem *>(droppedItemPtr);
    if (mapView == dropped->mapView)
    {
      auto focusedContainer = focusedItem.item->getDataAs<ItemData::Container>();

      mapView->history.beginTransaction(TransactionType::MoveItems);

      MapHistory::ContainerItemMoveInfo moveInfo;
      moveInfo.tile = mapView->getTile(focusedItem.position);
      moveInfo.item = focusedItem.item;
      moveInfo.containerIndex = std::min<size_t>(static_cast<size_t>(index), focusedContainer->size());

      mapView->moveFromMapToContainer(*dropped->tile, dropped->_item, moveInfo);

      mapView->history.endTransaction(TransactionType::MoveItems);

      // itemContainerModels.front()->refresh();
      containerModel->refresh(0);
    }
  }
  else if (util::hasDynamicType<ItemDrag::ContainerItemDrag *>(droppedItemPtr))
  {
    auto dropped = static_cast<ItemDrag::ContainerItemDrag *>(droppedItemPtr);
    VME_LOG_D("Received container item drop!");

    auto focusedContainer = focusedItem.item->getDataAs<ItemData::Container>();

    // Dropped on the same container slot that the drag started
    if (dropped->container() == focusedContainer && index == dropped->containerIndex)
    {
      return true;
    }

    MapHistory::ContainerItemMoveInfo from{};
    from.tile = mapView->getTile(dropped->position);
    from.item = dropped->_containerItem;
    from.containerIndex = dropped->containerIndex;

    MapHistory::ContainerItemMoveInfo to{};
    to.tile = mapView->getTile(focusedItem.position);
    to.item = focusedItem.item;
    to.containerIndex = std::min<size_t>(static_cast<size_t>(index), focusedContainer->size() - 1);

    mapView->history.beginTransaction(TransactionType::MoveItems);
    mapView->moveFromContainerToContainer(from, to);
    mapView->history.endTransaction(TransactionType::MoveItems);

    // itemContainerModels.front()->refresh();
    containerModel->refresh(0);
  }
  else
  {
    VME_LOG_D("[ItemPropertyWindow::itemDropEvent] What do we do here?");
    return false;
  }

  return true;
}

void ItemPropertyWindow::startContainerItemDrag(int index)
{
  VME_LOG_D("ItemPropertyWindow::startContainerItemDrag");

  auto focusedItem = state.focusedAs<FocusedItem>();

  ItemDrag::ContainerItemDrag itemDrag;
  itemDrag.position = focusedItem.position;
  itemDrag._containerItem = focusedItem.item;
  itemDrag.containerIndex = index;

  dragOperation.emplace(ItemDrag::DragOperation::create(std::move(itemDrag), state.mapView, this));
  dragOperation->setRenderCondition([this] { return !state.mapView->underMouse(); });
  dragOperation->start();
}

GuiItemContainer::ItemModel::ItemModel(QObject *parent)
    : QAbstractListModel(parent), _container(std::nullopt) {}

GuiItemContainer::ItemModel::ItemModel(ContainerItem containerItem, QObject *parent)
    : QAbstractListModel(parent), _container(std::nullopt)
{
  setContainer(containerItem);
}

void GuiItemContainer::ItemModel::setContainer(ContainerItem container)
{
  int oldCapacity = capacity();
  // VME_LOG_D("GuiItemContainer::ItemModel::setContainer capacity: " << container.containerCapacity());
  beginResetModel();
  _container.emplace(std::move(container));
  endResetModel();

  int newCapacity = capacity();

  if (newCapacity != oldCapacity)
  {
    emit capacityChanged(newCapacity);
  }
}

int GuiItemContainer::ItemModel::size()
{
  return _container ? static_cast<int>(_container->containerSize()) : 0;
}

int GuiItemContainer::ItemModel::capacity()
{
  return _container ? static_cast<int>(_container->containerCapacity()) : 0;
}

void GuiItemContainer::ItemModel::reset()
{
  if (!_container.has_value())
    return;

  VME_LOG_D("GuiItemContainer::ItemModel::reset");
  beginResetModel();
  _container.reset();
  endResetModel();

  emit capacityChanged(0);
}

bool GuiItemContainer::ItemModel::addItem(Item &&item)
{
  DEBUG_ASSERT(_container.has_value(), "Requires a container.");

  if (_container->full())
    return false;

  int size = static_cast<int>(_container->containerSize());

  ItemModel::createIndex(size, 0);
  ;

  // beginInsertRows(QModelIndex(), size, size + 1);
  bool added = _container->addItem(std::move(item));
  // endInsertRows();

  dataChanged(ItemModel::createIndex(size, 0), ItemModel::createIndex(size + 1, 0));

  return added;
}

int GuiItemContainer::ItemModel::rowCount(const QModelIndex &parent) const
{
  return _container ? static_cast<int>(_container->containerCapacity()) : 0;
}

QVariant GuiItemContainer::ItemModel::data(const QModelIndex &modelIndex, int role) const
{
  auto index = modelIndex.row();
  if (!_container || index < 0 || index >= rowCount())
    return QVariant();

  if (role == ServerIdRole)
  {
    if (index >= _container->containerSize())
    {
      return -1;
    }
    else
    {
      return _container->itemAt(index).serverId();
    }
  }

  return QVariant();
}

QHash<int, QByteArray> GuiItemContainer::ItemModel::roleNames() const
{
  QHash<int, QByteArray> roles;
  roles[ServerIdRole] = "serverId";

  return roles;
}

QPixmap ItemTypeImageProvider::requestPixmap(const QString &id, QSize *size, const QSize &requestedSize)
{
  bool success;
  auto serverId = id.toInt(&success);
  if (!success)
  {
    QPixmap pixmap(32, 32);
    pixmap.fill(QColor("black").rgba());
    return pixmap;
  }

  return QtUtil::itemPixmap(serverId);
}

//>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>ContainerModel>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>

GuiItemContainer::ContainerModel::ContainerModel(QObject *parent)
    : QAbstractListModel(parent) {}

void GuiItemContainer::ContainerModel::addContainerItem(ContainerItem containerItem)
{
  beginInsertRows(QModelIndex(), itemModels.size(), itemModels.size());
  itemModels.emplace_back(std::make_unique<ItemModel>(containerItem, this));
  endInsertRows();
  emit sizeChanged(size());
}

void GuiItemContainer::ContainerModel::remove(int index)
{
  beginRemoveRows(QModelIndex(), index, index);
  itemModels.erase(itemModels.begin() + index);
  endRemoveRows();
  emit sizeChanged(size());
}

int GuiItemContainer::ContainerModel::rowCount(const QModelIndex &parent) const
{
  return itemModels.size();
}

int GuiItemContainer::ContainerModel::size()
{
  return rowCount();
}

QVariant GuiItemContainer::ContainerModel::data(const QModelIndex &modelIndex, int role) const
{
  auto index = modelIndex.row();
  if (index < 0 || index >= rowCount())
    return QVariant();

  if (role == to_underlying(Role::ItemModel))
  {
    return QVariant::fromValue(itemModels.at(index).get());
  }

  return QVariant();
}

void GuiItemContainer::ContainerModel::clear()
{
  if (itemModels.empty())
    return;

  beginResetModel();
  itemModels.clear();
  endResetModel();
  emit sizeChanged(size());
}

void GuiItemContainer::ContainerModel::refresh(int index)
{
  itemModels.at(index)->refresh();
  auto modelIndex = createIndex(index, 0);
  dataChanged(modelIndex, modelIndex);
}

QHash<int, QByteArray> GuiItemContainer::ContainerModel::roleNames() const
{
  QHash<int, QByteArray> roles;
  roles[to_underlying(Role::ItemModel)] = "itemModel";

  return roles;
}
