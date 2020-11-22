#include "vulkan_window.h"

#include <algorithm>
#include <glm/gtc/type_ptr.hpp>
#include <memory>

#include <QAction>
#include <QDialog>
#include <QDrag>
#include <QFocusEvent>
#include <QMenu>
#include <QMimeData>
#include <QMouseEvent>
#include <QQuickView>
#include <QWidget>

#include "../logger.h"
#include "../main.h"
#include "../map_renderer.h"
#include "../map_view.h"
#include "../position.h"
#include "../qt/logging.h"
#include "gui.h"
#include "mainwindow.h"
#include "qt_util.h"

// Static
std::unordered_set<const VulkanWindow *> VulkanWindow::instances;

VulkanWindow::VulkanWindow(std::shared_ptr<Map> map, EditorAction &editorAction)
    : QVulkanWindow(nullptr),
      vulkanInfo(this),
      editorAction(editorAction),
      mapView(std::make_unique<MapView>(std::make_unique<QtUtil::QtUiUtils>(this), editorAction, map)),
      scrollAngleBuffer(0)
{
  instances.emplace(this);

  connect(this, &VulkanWindow::scrollEvent, [=](int scrollDelta) { this->mapView->zoom(scrollDelta); });
  mapView->onMapItemDragStart<&VulkanWindow::mapItemDragStartEvent>(this);

  setShortcut(Qt::Key_Z | Qt::CTRL, ShortcutAction::Undo);
  setShortcut(Qt::Key_Z | Qt::CTRL | Qt::SHIFT, ShortcutAction::Redo);
  setShortcut(Qt::Key_Space, ShortcutAction::Pan);
  setShortcut(Qt::Key_I, ShortcutAction::EyeDropper);
  setShortcut(Qt::Key_Escape, ShortcutAction::Escape);
  setShortcut(Qt::Key_Delete, ShortcutAction::Delete);
  setShortcut(Qt::Key_0 | Qt::CTRL, ShortcutAction::ResetZoom);
  setShortcut(Qt::Key_Plus | Qt::KeypadModifier, ShortcutAction::FloorUp);
  setShortcut(Qt::Key_Minus | Qt::KeypadModifier, ShortcutAction::FloorDown);
  setShortcut(Qt::Key_Q, ShortcutAction::LowerFloorShade);
}

VulkanWindow::~VulkanWindow()
{
  instances.erase(this);
}

void VulkanWindow::shortcutPressedEvent(ShortcutAction action, QKeyEvent *event)
{
  switch (action)
  {
  case ShortcutAction::Undo:
    mapView->undo();
    break;
  case ShortcutAction::Redo:
    mapView->redo();
    break;
  case ShortcutAction::Pan:
  {
    bool panning = mapView->editorAction.is<MouseAction::Pan>();
    if (panning || QApplication::mouseButtons() & Qt::MouseButton::LeftButton)
    {
      break;
    }

    setCursor(Qt::OpenHandCursor);

    MouseAction::Pan pan;
    mapView->editorAction.setIfUnlocked(pan);
    mapView->requestDraw();
    break;
  }
  case ShortcutAction::EyeDropper:
  {
    const Item *topItem = mapView->map()->getTopItem(mapView->mouseGamePos());
    if (topItem && !mapView->editorAction.locked())
    {
      mapView->editorAction.setRawItem(topItem->serverId());
      mapView->requestDraw();
    }
    break;
  }
  case ShortcutAction::Escape:
    mapView->escapeEvent();
    break;
  case ShortcutAction::Delete:
    mapView->deleteSelectedItems();
    break;
  case ShortcutAction::ResetZoom:
    mapView->resetZoom();
    break;
  case ShortcutAction::FloorUp:
    mapView->floorUp();
    break;
  case ShortcutAction::FloorDown:
    mapView->floorDown();

    break;
  case ShortcutAction::LowerFloorShade:
    mapView->toggleViewOption(MapView::ViewOption::ShadeLowerFloors);
    break;
  }
}

//>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>Events>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>

void VulkanWindow::mapItemDragStartEvent(Tile *tile, Item *item)
{
  ItemDrag::MapItem mapItem(mapView.get(), tile, item);
  dragOperation.emplace(ItemDrag::DragOperation::create(std::move(mapItem), this));
  dragOperation->setRenderCondition([this] { return !this->containsMouse(); });
  dragOperation->start();
}

void VulkanWindow::shortcutReleasedEvent(ShortcutAction action, QKeyEvent *event)
{
  switch (action)
  {
  case ShortcutAction::Pan:
    auto pan = mapView->editorAction.as<MouseAction::Pan>();
    if (pan)
    {
      unsetCursor();
      mapView->editorAction.setPrevious();
    }
    break;
  }
}

void VulkanWindow::mousePressEvent(QMouseEvent *event)
{
  bool mouseInside = containsMouse();
  mapView->setUnderMouse(mouseInside);
  VME_LOG_D("VulkanWindow::mousePressEvent");
  mouseState.buttons = event->buttons();

  switch (event->button())
  {
  case Qt::MouseButton::RightButton:
    showContextMenu(event->globalPos());
    break;
  case Qt::MouseButton::LeftButton:
    if (contextMenu)
    {
      closeContextMenu();
    }
    else
    {
      mapView->mousePressEvent(QtUtil::vmeMouseEvent(event));
    }

    break;
  default:
    break;
  }

  event->ignore();
}

void VulkanWindow::mouseMoveEvent(QMouseEvent *event)
{
  bool mouseInside = containsMouse();
  mapView->setUnderMouse(mouseInside);

  mouseState.buttons = event->buttons();
  mapView->mouseMoveEvent(QtUtil::vmeMouseEvent(event));

  // Handle external drag operation
  if (dragOperation)
  {
    dragOperation->mouseMoveEvent(event);
  }

  auto pos = event->windowPos();
  util::Point<float> mousePos(pos.x(), pos.y());
  emit mousePosChanged(mousePos);

  event->ignore();
  QVulkanWindow::mouseMoveEvent(event);
}

void VulkanWindow::mouseReleaseEvent(QMouseEvent *event)
{
  // Propagate drag operation
  if (dragOperation)
  {
    bool accepted = dragOperation->sendDropEvent(event);
    if (accepted)
    {
      // Only remove the item from the map if it was not dropped back onto the map.
      if (!containsMouse())
      {
        const auto &draggedItem = dragOperation->mimeData.draggableItem;
        draggedItem->remove();
      }
    }
    else
    {
      mapView->editorAction.as<MouseAction::DragDropItem>()->moveDelta.emplace(PositionConstants::Zero);
    }

    dragOperation.reset();
  }

  mouseState.buttons = event->buttons();
  mapView->mouseReleaseEvent(QtUtil::vmeMouseEvent(event));
}

void VulkanWindow::wheelEvent(QWheelEvent *event)
{
  /*
    The minimum rotation amount for a scroll to be registered, in eighths of a degree.
    For example, 120 MinRotationAmount = (120 / 8) = 15 degrees of rotation.
  */
  const int MinRotationAmount = 120;

  // The relative amount that the wheel was rotated, in eighths of a degree.
  const int deltaY = event->angleDelta().y();

  scrollAngleBuffer += deltaY;
  if (std::abs(scrollAngleBuffer) >= MinRotationAmount)
  {
    emit scrollEvent(scrollAngleBuffer / 8);
    scrollAngleBuffer = 0;
  }
}

void VulkanWindow::keyReleaseEvent(QKeyEvent *e)
{
  if (e->isAutoRepeat())
    return;

  auto shortcut = shortcuts.find(e->key() | e->modifiers());
  if (shortcut != shortcuts.end())
  {
    shortcutReleasedEvent(shortcut->second);
    return;
  }

  switch (e->key())
  {
  case Qt::Key_Control:
  {
    auto rawItem = mapView->editorAction.as<MouseAction::RawItem>();
    if (rawItem)
    {
      rawItem->erase = false;
    }
  }
  }
}

void VulkanWindow::keyPressEvent(QKeyEvent *e)
{
  switch (e->key())
  {
  case Qt::Key_Left:
  case Qt::Key_Right:
  case Qt::Key_Up:
  case Qt::Key_Down:
    e->ignore();
    emit keyPressedEvent(e);
    break;
  case Qt::Key_Control:
  {
    auto rawItemAction = mapView->editorAction.as<MouseAction::RawItem>();
    if (rawItemAction)
    {
      rawItemAction->erase = true;
    }
    break;
  }
  default:
    e->ignore();
    QVulkanWindow::keyPressEvent(e);
    break;
  }
}

void VulkanWindow::setShortcut(int keyAndModifiers, ShortcutAction shortcut)
{
  shortcuts.emplace(keyAndModifiers, shortcut);
  shortcutActionToKeyCombination.emplace(shortcut, keyAndModifiers);
}

std::optional<ShortcutAction> VulkanWindow::getShortcutAction(QKeyEvent *event) const
{
  auto shortcut = shortcuts.find(event->key() | event->modifiers());

  return shortcut != shortcuts.end() ? std::optional<ShortcutAction>(shortcut->second) : std::nullopt;
}

void VulkanWindow::dropEvent(QDropEvent *event)
{
  if (event->mimeData()->hasFormat(ItemDrag::DragOperation::MapItemFormat))
  {
    event->accept();
  }
}

bool VulkanWindow::event(QEvent *event)
{
  // if (!(event->type() == QEvent::UpdateRequest) && !(event->type() == QEvent::MouseMove))
  // {
  //   qDebug() << "[" << QString(debugName.c_str()) << "] " << event->type() << " { " << mapToGlobal(position()) << " }";
  // }

  switch (event->type())
  {
  case QEvent::Leave:
  case QEvent::DragLeave:
    mapView->setUnderMouse(false);
    break;
  case QEvent::Enter:
  case QEvent::DragEnter:
    mapView->setUnderMouse(true);
    break;
  case QEvent::Drop:
    dropEvent(static_cast<QDropEvent *>(event));
    break;
  case QEvent::ShortcutOverride:
  {
    QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
    auto action = getShortcutAction(keyEvent);
    if (action)
    {
      shortcutPressedEvent(action.value());
      return true;
    }
    break;
  }
  default:
    event->ignore();
    break;
  }

  return QVulkanWindow::event(event);
}

void VulkanWindow::lostFocus()
{
  if (contextMenu)
  {
    closeContextMenu();
  }
}

QWidget *VulkanWindow::wrapInWidget(QWidget *parent)
{
  QWidget *wrapper = QWidget::createWindowContainer(this, parent);
  QtUtil::setMapView(*wrapper, mapView.get());
  QtUtil::setVulkanWindow(*wrapper, this);
  wrapper->setObjectName("VulkanWindow wrapper");

  widget = wrapper;

  return wrapper;
}

QVulkanWindowRenderer *VulkanWindow::createRenderer()
{
  if (!renderer)
  {
    // Memory deleted by QT when QT closes
    renderer = new VulkanWindow::Renderer(*this);
  }

  return renderer;
}

QRect VulkanWindow::localGeometry() const
{
  return QRect(QPoint(0, 0), QPoint(width(), height()));
}

void VulkanWindow::closeContextMenu()
{
  VME_LOG_D("VulkanWindow::closeContextMenu");
  contextMenu->close();
  contextMenu = nullptr;
}

void VulkanWindow::showContextMenu(QPoint position)
{
  if (contextMenu)
  {
    closeContextMenu();
  }

  ContextMenu *menu = new ContextMenu(this, widget);
  // widget->setStyleSheet("background-color:green;");

  QAction *cut = new QAction(tr("Cut"), menu);
  cut->setShortcut(Qt::CTRL + Qt::Key_X);
  menu->addAction(cut);

  QAction *copy = new QAction(tr("Copy"), menu);
  copy->setShortcut(Qt::CTRL + Qt::Key_C);
  menu->addAction(copy);

  QAction *paste = new QAction(tr("Paste"), menu);
  paste->setShortcut(Qt::CTRL + Qt::Key_V);
  menu->addAction(paste);

  QAction *del = new QAction(tr("Delete"), menu);
  del->setShortcut(Qt::Key_Delete);
  menu->addAction(del);

  this->contextMenu = menu;

  menu->connect(menu, &QMenu::aboutToHide, [this] {
    this->contextMenu = nullptr;
  });
  menu->popup(position);
}

MapView *VulkanWindow::getMapView() const
{
  return mapView.get();
}

util::Size VulkanWindow::vulkanSwapChainImageSize() const
{
  QSize size = swapChainImageSize();
  return util::Size(size.width(), size.height());
}

void VulkanWindow::updateVulkanInfo()
{
  vulkanInfo.update();
}

bool VulkanWindow::containsMouse() const
{
  QSize windowSize = size();
  auto mousePos = mapFromGlobal(QCursor::pos());

  return (0 <= mousePos.x() && mousePos.x() <= windowSize.width()) && (0 <= mousePos.y() && mousePos.y() <= windowSize.height());
}

//>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>ContextMenu>>>>>
//>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>

VulkanWindow::ContextMenu::ContextMenu(VulkanWindow *window, QWidget *widget) : QMenu(widget)
{
}

bool VulkanWindow::ContextMenu::selfClicked(QPoint pos) const
{
  return localGeometry().contains(pos);
}

void VulkanWindow::ContextMenu::mousePressEvent(QMouseEvent *event)
{
  event->ignore();
  QMenu::mousePressEvent(event);

  // // Propagate the click event to the map window if appropriate
  // if (!selfClicked(event->pos()))
  // {
  //   auto posInWindow = window->mapFromGlobal(event->globalPos());
  //   VME_LOG_D("posInWindow: " << posInWindow);
  //   VME_LOG_D("Window geometry: " << window->geometry());
  //   if (window->localGeometry().contains(posInWindow.x(), posInWindow.y()))
  //   {
  //     VME_LOG_D("In window");
  //     window->mousePressEvent(event);
  //   }
  //   else
  //   {
  //     event->ignore();
  //     window->lostFocus();
  //   }
  // }
}

QRect VulkanWindow::ContextMenu::localGeometry() const
{
  return QRect(QPoint(0, 0), QPoint(width(), height()));
}
QRect VulkanWindow::ContextMenu::relativeGeometry() const
{
  VME_LOG_D("relativeGeometry");
  //  VME_LOG_D(parentWidget()->pos());
  QPoint p(geometry().left(), geometry().top());

  VME_LOG_D(parentWidget()->mapToGlobal(parentWidget()->pos()));

  VME_LOG_D("Top left: " << p);
  VME_LOG_D(mapToParent(p));

  return geometry();
}

//>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>Renderer>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>

VulkanWindow::Renderer::Renderer(VulkanWindow &window)
    : window(window),
      renderer(window.vulkanInfo, window.mapView.get()) {}

void VulkanWindow::Renderer::initResources()
{
  renderer.initResources(window.colorFormat());
};

void VulkanWindow::Renderer::initSwapChainResources()
{
  renderer.initSwapChainResources(window.vulkanSwapChainImageSize());
};

void VulkanWindow::Renderer::releaseSwapChainResources()
{
  renderer.releaseSwapChainResources();
};

void VulkanWindow::Renderer::releaseResources()
{
  renderer.releaseResources();
};

void VulkanWindow::Renderer::startNextFrame()
{
  renderer.setCurrentFrame(window.currentFrame());
  auto frame = renderer.currentFrame();
  frame->currentFrameIndex = window.currentFrame();
  frame->commandBuffer = window.currentCommandBuffer();
  frame->frameBuffer = window.currentFramebuffer();
  frame->mouseAction = window.mapView->editorAction.action();

  renderer.startNextFrame();
}
