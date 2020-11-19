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
  auto model = new QtItemTest::ItemModel();

  // auto k = item.as<ContainerItem>();

  std::vector<uint32_t> serverIds{{1987, 2148, 5710, 2673, 2463, 2649}};
  // std::vector<uint32_t> serverIds{{4526, 103, 4566, 7062, 3216, 6580, 4526, 103, 4566, 7062, 3216, 6580}};
  // for (const auto id : serverIds)
  //   model->addItem(new Item(id));

  auto bag = new Item(1987);
  ItemData::Container container;
  for (const auto id : serverIds)
    container.addItem(Item(id));

  bag->setItemData(std::move(container));

  model->setContainer(bag->as<ContainerItem>());

  setInitialProperties({{"containerItems", QVariant::fromValue(model)}});

  engine()->rootContext()->setContextProperty("propertyWindow", this);
  engine()->addImageProvider(QLatin1String("itemTypes"), new ItemTypeImageProvider);

  setSource(_url);

  QmlApplicationContext *applicationContext = new QmlApplicationContext();
  engine()->rootContext()->setContextProperty("applicationContext", applicationContext);
}

void ItemPropertyWindow::setItem(const Item &item)
{
  setContainerVisible(item.isContainer());
  setCount(item.count());
}

void ItemPropertyWindow::resetItem()
{
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

QtItemTest::ItemWrapper::ItemWrapper(Item *item)
    : item(item)
{
}

int QtItemTest::ItemWrapper::serverId() const
{
  return hasItem() ? item->serverId() : -1;
}

bool QtItemTest::ItemWrapper::hasItem() const noexcept
{
  return item != nullptr;
}

QtItemTest::ItemModel::ItemModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

void QtItemTest::ItemModel::setContainer(ContainerItem &container)
{
  _itemWrappers.clear();
  size_t size = container.containerSize();
  for (int i = 0; i < container.containerCapacity(); ++i)
  {
    if (i < size)
      addItem(&container.container()->itemAt(i));
    else
      addItem(nullptr);
  }
}

void QtItemTest::ItemModel::addItem(const ItemWrapper &item)
{
  beginInsertRows(QModelIndex(), rowCount(), rowCount() + 1);
  _itemWrappers.emplace_back(item);
  endInsertRows();
}

int QtItemTest::ItemModel::rowCount(const QModelIndex &parent) const
{
  return _itemWrappers.size();
}

QVariant QtItemTest::ItemModel::data(const QModelIndex &index, int role) const
{
  auto row = index.row();
  if (row < 0 || row >= _itemWrappers.size())
    return QVariant();

  const ItemWrapper &itemWrapper = _itemWrappers.at(row);
  if (role == ServerIdRole)
  {
    return itemWrapper.hasItem() ? itemWrapper.serverId() : -1;
  }

  return QVariant();
}

QHash<int, QByteArray> QtItemTest::ItemModel::roleNames() const
{
  QHash<int, QByteArray> roles;
  roles[ServerIdRole] = "serverId";

  return roles;
}

//>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>QML Callbacks>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>

void ItemPropertyWindow::itemDropEvent(QByteArray array)
{

  auto mapItem = ItemDragOperation::MimeData::MapItem::fromByteArray(array);
  Item item(mapItem.moveFromMap());
  VME_LOG_D("Dropping: " << item.name() << ", (" << item.serverId() << ")");
}
