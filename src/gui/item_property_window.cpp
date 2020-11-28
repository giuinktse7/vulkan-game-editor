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

  // itemContainerModels.emplace_back(std::make_unique<GuiItemContainer::ItemModel>());

  containerTree.onContainerItemDrop<&ItemPropertyWindow::itemDropEvent>(this);

  QVariantMap properties;
  properties.insert("containers", QVariant::fromValue(&containerTree.containerModel));
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
  containerTree.clear();

  setCount(1);

  FocusedGround ground;
  ground.position = position;
  ground.ground = mapView.getTile(position)->ground();

  DEBUG_ASSERT(ground.ground != nullptr, "Can not focus nullptr ground.");

  state.focusedItem = ground;
}

void ItemPropertyWindow::focusItem(Item &item, Position &position, MapView &mapView)
{
  if (item.isGround())
  {
    focusGround(position, mapView);
    return;
  }
  else if (state.holds<FocusedItem>() && &item == state.focusedAs<FocusedItem>().item)
  {
    // The item is already focused
    return;
  }

  setMapView(mapView);

  FocusedItem focusedItem;

  auto maybeTileIndex = mapView.getTile(position)->indexOf(&item);
  DEBUG_ASSERT(maybeTileIndex.has_value(), "The tile did not have the item.");
  auto tileIndex = maybeTileIndex.value();

  bool isContainer = item.isContainer();

  if (isContainer)
  {
    if (containerTree.rootItem() == &item)
    {
      // This is already the focused item.
      return;
    }
    auto container = ContainerItem::wrap(item).value();
    if (container.empty())
    {
      std::vector<uint32_t> serverIds{{1987, 2148, 5710, 2673, 2463, 2649}};

      for (const auto id : serverIds)
        container.addItem(Item(id));
    }

    containerTree.setRootContainer(position, tileIndex, container);
  }

  setContainerVisible(isContainer);
  setCount(item.count());

  focusedItem.item = &item;
  focusedItem.tileIndex = tileIndex;
  focusedItem.position = position;

  state.focusedItem = focusedItem;
}

void ItemPropertyWindow::resetFocus()
{
  // itemContainerModels.front()->reset();
  containerTree.clear();
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

void GuiItemContainer::ItemModel::refresh()
{
  beginResetModel();
  endResetModel();
}

void ItemPropertyWindow::refresh()
{
  // itemContainerModels.front()->refresh();
  if (containerTree.hasRoot())
  {
    auto containerArea = child(ObjectName::ItemContainerArea);
    if (containerArea->property("visible").toBool())
    {
      containerTree.containerModel.refresh(0);
    }
  }
}

bool ItemPropertyWindow::itemDropEvent(GuiItemContainer::ContainerNode *treeNode, int index, const ItemDrag::DraggableItem *droppedItem)
{
  auto &focusedItem = state.focusedAs<FocusedItem>();
  if (droppedItem->item() == focusedItem.item)
  {
    VME_LOG_D("Can not add item to itself.");
    return false;
  }

  auto mapView = state.mapView;

  if (util::hasDynamicType<ItemDrag::MapItem *>(droppedItem))
  {
    auto dropped = static_cast<const ItemDrag::MapItem *>(droppedItem);
    if (mapView == dropped->mapView)
    {
      auto container = treeNode->container.container();

      mapView->history.beginTransaction(TransactionType::MoveItems);

      // TODO Create a history move action that moves into a container nested within another (possibly multiple) containers.
      MapHistory::ContainerItemMoveInfo moveInfo;
      moveInfo.tile = mapView->getTile(focusedItem.position);
      moveInfo.item = treeNode->container.item;
      moveInfo.containerIndex = std::min<size_t>(static_cast<size_t>(index), container->size());

      mapView->moveFromMapToContainer(*dropped->tile, dropped->_item, moveInfo);

      mapView->history.endTransaction(TransactionType::MoveItems);

      // itemContainerModels.front()->refresh();

      containerTree.containerModel.refresh(treeNode->model());
    }
  }
  else if (util::hasDynamicType<ItemDrag::ContainerItemDrag *>(droppedItem))
  {
    auto dropped = static_cast<const ItemDrag::ContainerItemDrag *>(droppedItem);
    VME_LOG_D("Received container item drop!");

    if (dropped->mapView != state.mapView)
    {
      ABORT_PROGRAM("Drag between different MapViews is not implemented.");
    }

    auto targetContainer = treeNode->container.container();

    // Dropped on the same container slot that the drag started
    if (dropped->container() == targetContainer && index == dropped->containerIndices.back())
    {
      return true;
    }

    MapHistory::ContainerMoveData2 from(
        dropped->position,
        dropped->tileIndex,
        std::move(dropped->containerIndices));

    // TODO Use correct indices
    std::vector<uint16_t> toIndices{0};

    MapHistory::ContainerMoveData2 to(
        focusedItem.position,
        focusedItem.tileIndex,
        toIndices);

    mapView->history.beginTransaction(TransactionType::MoveItems);
    mapView->moveFromContainerToContainer(from, to);
    mapView->history.endTransaction(TransactionType::MoveItems);

    // itemContainerModels.front()->refresh();
    containerTree.containerModel.refresh(0);
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
  itemDrag.mapView = state.mapView;
  itemDrag.position = focusedItem.position;
  // TODO Use correct indices
  itemDrag.containerIndices = std::vector<uint16_t>{static_cast<uint16_t>(index)};
  itemDrag.tileIndex = focusedItem.tileIndex;

  dragOperation.emplace(ItemDrag::DragOperation::create(std::move(itemDrag), state.mapView, this));
  dragOperation->setRenderCondition([this] { return !state.mapView->underMouse(); });
  dragOperation->start();
}

GuiItemContainer::ItemModel::ItemModel(ContainerNode *treeNode, QObject *parent)
    : QAbstractListModel(parent), treeNode(treeNode)
{
  // TODO: Maybe this reset is not necessary?
  beginResetModel();
  endResetModel();
}

void GuiItemContainer::ItemModel::containerItemClicked(int index)
{
  VME_LOG_D("containerItemClicked. Item id: " << container()->item->serverId() << ", index: " << index);
  if (container()->itemAt(index).isContainer())
  {
    treeNode->toggleChild(index);
  }
}

bool GuiItemContainer::ItemModel::itemDropEvent(int index, QByteArray serializedDraggableItem)
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

  if (droppedItem->item() == container()->item)
  {
    VME_LOG_D("Can not add item to itself.");
    return false;
  }

  treeNode->itemDropEvent(index, droppedItem.get());
  return true;
}

int GuiItemContainer::ItemModel::size()
{
  return static_cast<int>(treeNode->container.containerSize());
}

int GuiItemContainer::ItemModel::capacity()
{
  return static_cast<int>(treeNode->container.containerCapacity());
}

const ContainerItem *GuiItemContainer::ItemModel::container() const noexcept
{
  return &treeNode->container;
}

ContainerItem *GuiItemContainer::ItemModel::container() noexcept
{
  return const_cast<ContainerItem *>(const_cast<const GuiItemContainer::ItemModel *>(this)->container());
}

bool GuiItemContainer::ItemModel::addItem(Item &&item)
{
  if (container()->full())
    return false;

  int size = static_cast<int>(container()->containerSize());

  ItemModel::createIndex(size, 0);
  ;

  // beginInsertRows(QModelIndex(), size, size + 1);
  bool added = container()->addItem(std::move(item));
  // endInsertRows();

  dataChanged(ItemModel::createIndex(size, 0), ItemModel::createIndex(size + 1, 0));

  return added;
}

int GuiItemContainer::ItemModel::rowCount(const QModelIndex &parent) const
{
  return container()->containerCapacity();
}

QVariant GuiItemContainer::ItemModel::data(const QModelIndex &modelIndex, int role) const
{
  auto index = modelIndex.row();
  if (index < 0 || index >= rowCount())
    return QVariant();

  if (role == ServerIdRole)
  {
    if (index >= container()->containerSize())
    {
      return -1;
    }
    else
    {
      return container()->itemAt(index).serverId();
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

std::vector<GuiItemContainer::ItemModel *>::iterator GuiItemContainer::ContainerModel::find(const ItemModel *model)
{
  return std::find_if(
      itemModels.begin(),
      itemModels.end(),
      [model](const ItemModel *_model) { return model == _model; });
}

void GuiItemContainer::ContainerModel::addItemModel(ItemModel *model)
{
  auto modelSize = static_cast<int>(itemModels.size());
  beginInsertRows(QModelIndex(), modelSize, modelSize);
  itemModels.emplace_back(model);
  endInsertRows();
  emit sizeChanged(size());
}

void GuiItemContainer::ContainerModel::remove(ItemModel *model)
{
  auto found = std::remove_if(
      itemModels.begin(),
      itemModels.end(),
      [model](const ItemModel *_model) { return model == _model; });

  if (found == itemModels.end())
  {
    VME_LOG_D("GuiItemContainer::ContainerModel::remove: ItemModel '" << model << "' was not present.");
    return;
  }

  itemModels.erase(found);
}

void GuiItemContainer::ContainerModel::refresh(ItemModel *model)
{
  auto found = find(model);
  DEBUG_ASSERT(found != itemModels.end(), "model was not present.");

  (*found)->refresh();
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
  return static_cast<int>(itemModels.size());
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
    return QVariant::fromValue(itemModels.at(index));
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

//>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>ContainterTree>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>
using ContainterTree = GuiItemContainer::ContainerTree;

GuiItemContainer::ContainerTree::ContainerTree()
{
  _signals.postOpened.connect<&ContainerTree::modelAddedEvent>(this);
  _signals.preClosed.connect<&ContainerTree::modelRemovedEvent>(this);
}

GuiItemContainer::ContainerTree::Root::Root(
    Position mapPosition,
    uint16_t tileIndex,
    ContainerItem container,
    ContainerTreeSignals *_signals)
    : ContainerNode(container, _signals),
      mapPosition(mapPosition),
      tileIndex(tileIndex) {}

const Item *ContainterTree::rootItem() const
{
  return root ? root->container.item : nullptr;
}

bool ContainterTree::hasRoot() const noexcept
{
  return root.has_value();
}

void ContainterTree::setRootContainer(Position position, uint16_t tileIndex, ContainerItem item)
{
  root.emplace(position, tileIndex, item, &_signals);
  root->open();
}

void ContainterTree::clear()
{
  root.reset();
  containerModel.clear();
}

void ContainterTree::modelAddedEvent(ItemModel *model)
{
  containerModel.addItemModel(model);
}

void ContainterTree::modelRemovedEvent(ItemModel *model)
{
  containerModel.remove(model);
}

GuiItemContainer::ContainerNode::ContainerNode(ContainerItem container, ContainerTreeSignals *_signals)
    : container(container), _signals(_signals)
{
  container.container()->on_destroyed<&ContainerNode::containerDestroyedEvent>(this);
}

GuiItemContainer::ContainerNode::ContainerNode(ContainerItem container, ContainerNode *parent)
    : container(container), _signals(parent->_signals)
{
  container.container()->on_destroyed<&ContainerNode::containerDestroyedEvent>(this);
}

void GuiItemContainer::ContainerNode::containerDestroyedEvent()
{
  VME_LOG_D("Container destroyed.");
}

GuiItemContainer::ContainerNode::~ContainerNode()
{
  if (opened)
  {
    close();
  }
}

GuiItemContainer::ItemModel *GuiItemContainer::ContainerNode::model()
{
  return _model.has_value() ? &(*_model) : nullptr;
}

void GuiItemContainer::ContainerNode::open()
{
  DEBUG_ASSERT(!opened, "Already opened.");

  _model.emplace(this);
  _signals->postOpened.fire(&_model.value());
  opened = true;
}

void GuiItemContainer::ContainerNode::close()
{
  _signals->preClosed.fire(&_model.value());
  _model.reset();
  opened = false;
}

void GuiItemContainer::ContainerNode::toggle()
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

void GuiItemContainer::ContainerNode::openChild(int index)
{
  DEBUG_ASSERT(children.find(index) == children.end(), "The child is already opened.");
  auto &child = container.itemAt(index);

  DEBUG_ASSERT(child.isContainer(), "Must be container.");
  auto containerItem = ContainerItem::wrap(container.itemAt(index)).value();
  auto node = std::make_unique<ContainerTree::Node>(std::move(containerItem), this, index);
  children.emplace(index, std::move(node));
  children.at(index)->open();
}

void GuiItemContainer::ContainerNode::toggleChild(int index)
{
  auto child = children.find(index);
  if (child == children.end())
  {
    openChild(index);
    return;
  }
  child->second->toggle();
}

void GuiItemContainer::ContainerNode::itemDropEvent(int index, ItemDrag::DraggableItem *droppedItem)
{
  // TODO Maybe use fire_accumulate here to see if drop was accepted.
  //Drop **should** always be accepted for now, but that might change in the future.

  _signals->itemDropped.fire(this, index, droppedItem);
}
