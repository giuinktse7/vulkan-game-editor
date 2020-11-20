#include "item_property_window.h"

#include <QQmlContext>
#include <QQmlEngine>
#include <QQmlProperty>
#include <QWidget>

#include "vulkan_window.h"

namespace ObjectName
{
  constexpr auto CountSpinBox = "count_spinbox";
  constexpr auto ActionIdSpinBox = "action_id_spinbox";
  constexpr auto UniqueIdSpinBox = "unique_id_spinbox";

  constexpr auto ItemContainerArea = "item_container_area";
} // namespace ObjectName

bool PropertyWindowEventFilter::eventFilter(QObject *obj, QEvent *event)
{
  // qDebug() << "PropertyWindowEventFilter:" << event;
  return false;

  // return QObject::eventFilter(obj, event);
}

ItemPropertyWindow::ItemPropertyWindow(QUrl url)
    : _url(url)
{
  installEventFilter(new PropertyWindowEventFilter(this));
  itemContainerModel = new GuiItemContainer::ItemModel();

  setInitialProperties({{"containerItems", QVariant::fromValue(itemContainerModel)}});

  engine()->rootContext()->setContextProperty("propertyWindow", this);
  engine()->addImageProvider(QLatin1String("itemTypes"), new ItemTypeImageProvider);

  setSource(_url);

  QmlApplicationContext *applicationContext = new QmlApplicationContext();
  engine()->rootContext()->setContextProperty("applicationContext", applicationContext);
}

void ItemPropertyWindow::setItem(Item &item)
{
  bool isContainer = item.isContainer();
  if (isContainer)
  {
    auto container = ContainerItem::wrap(item);
    if (container->empty())
    {
      std::vector<uint32_t> serverIds{{1987, 2148, 5710, 2673, 2463, 2649}};

      for (const auto id : serverIds)
        container->addItem(Item(id));
    }

    itemContainerModel->setContainer(std::move(container.value()));
  }
  else
  {
    itemContainerModel->reset();
  }

  setContainerVisible(isContainer);
  setCount(item.count());
}

void ItemPropertyWindow::resetItem()
{
  VME_LOG_D("ResetItem");
  itemContainerModel->reset();
  setContainerVisible(false);
  setCount(1);
}

void ItemPropertyWindow::setCount(uint8_t count)
{
  auto countSpinBox = child(ObjectName::CountSpinBox);
  countSpinBox->setProperty("value", count);
}

void ItemPropertyWindow::setContainerVisible(bool visible)
{
  auto containerArea = child(ObjectName::ItemContainerArea);
  containerArea->setProperty("visible", visible);
}

QWidget *ItemPropertyWindow::wrapInWidget(QWidget *parent)
{
  QWidget *wrapper = QWidget::createWindowContainer(this, parent);
  wrapper->setObjectName("ItemPropertyWindow wrapper");

  return wrapper;
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

void ItemPropertyWindow::itemDropEvent(int index, QByteArray array)
{
  VME_LOG_D("Index: " << index);
  auto mapItem = ItemDragOperation::MimeData::MapItem::fromByteArray(array);
  if (!mapItem)
  {
    VME_LOG("[Warning]: Could not read MapItem from qml QByteArray.");
    return;
  }

  Item item(mapItem.value().moveFromMap());
  VME_LOG_D("Dropping: " << item.name() << ", (" << item.serverId() << ")");
  // itemContainerModel->
}

//>>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>GuiItemContainer>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>>

// GuiItemContainer::ItemWrapper::ItemWrapper(Item *item)
//     : item(item)
// {
// }

// int GuiItemContainer::ItemWrapper::serverId() const
// {
//   return hasItem() ? item->serverId() : -1;
// }

// bool GuiItemContainer::ItemWrapper::hasItem() const noexcept
// {
//   return item != nullptr;
// }

GuiItemContainer::ItemModel::ItemModel(QObject *parent)
    : QAbstractListModel(parent), _container(std::nullopt) {}

void GuiItemContainer::ItemModel::setContainer(ContainerItem &&container)
{
  int oldCapacity = capacity();
  VME_LOG_D("GuiItemContainer::ItemModel::setContainer capacity: " << container.containerCapacity());
  // beginInsertRows(QModelIndex(), 0, container.containerCapacity() - 1);
  beginResetModel();
  _container.emplace(std::move(container));
  endResetModel();
  // endInsertRows();

  int newCapacity = capacity();

  if (newCapacity != oldCapacity)
  {
    emit capacityChanged(newCapacity);
  }
}

int GuiItemContainer::ItemModel::capacity()
{
  return _container ? _container->containerCapacity() : 0;
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

void GuiItemContainer::ItemModel::addItem(Item &&item)
{
  DEBUG_ASSERT(_container.has_value(), "Requires a container.");
  auto size = _container->containerSize();
  beginInsertRows(QModelIndex(), size, size + 1);
  _container->addItem(std::move(item));
  endInsertRows();
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
