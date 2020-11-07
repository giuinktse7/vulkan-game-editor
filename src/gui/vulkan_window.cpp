#include "vulkan_window.h"

#include <QAction>
#include <QDialog>
#include <QFocusEvent>
#include <QMenu>
#include <QMouseEvent>

#include <memory>

#include <glm/gtc/type_ptr.hpp>

#include "../map_renderer.h"
#include "../map_view.h"

#include "../logger.h"
#include "../position.h"

#include "../qt/logging.h"
#include "qt_util.h"

#include "gui.h"

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
}

VulkanWindow::~VulkanWindow()
{
  instances.erase(this);
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

void VulkanWindow::mousePressEvent(QMouseEvent *e)
{
  VME_LOG_D("VulkanWindow::mousePressEvent");
  Qt::MouseButton button = e->button();
  switch (button)
  {
  case Qt::MouseButton::RightButton:
    showContextMenu(e->globalPos());
    break;
  case Qt::MouseButton::LeftButton:
    if (contextMenu)
    {
      closeContextMenu();
    }
    else
    {
      mapView->mousePressEvent(QtUtil::vmeMouseEvent(e));
    }

    break;
  default:
    break;
  }

  e->ignore();
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

void VulkanWindow::mouseReleaseEvent(QMouseEvent *event)
{
  mapView->mouseReleaseEvent(QtUtil::vmeMouseEvent(event));
}

void VulkanWindow::mouseMoveEvent(QMouseEvent *event)
{
  mapView->mouseMoveEvent(QtUtil::vmeMouseEvent(event));

  auto pos = event->windowPos();
  util::Point<float> mousePos(pos.x(), pos.y());
  emit mousePosChanged(mousePos);

  event->ignore();
  QVulkanWindow::mouseMoveEvent(event);
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

  switch (e->key())
  {
  case Qt::Key_Space:
  {
    auto pan = mapView->editorAction.as<MouseAction::Pan>();
    if (pan)
    {
      unsetCursor();
      mapView->editorAction.setPrevious();
    }
    break;
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
  case Qt::Key_Escape:
    mapView->escapeEvent();
    break;
  case Qt::Key_Delete:
    mapView->deleteSelectedItems();
    break;
  case Qt::Key_0:
    if (e->modifiers() & Qt::CTRL)
    {
      mapView->resetZoom();
    }
    break;
  case Qt::Key_I:
  {
    const Item *topItem = mapView->map()->getTopItem(mapView->mouseGamePos());
    if (topItem)
    {
      mapView->editorAction.setRawItem(topItem->serverId());
    }
    break;
  }
  case Qt::Key_Space:
  {
    if (!mapView->editorAction.is<MouseAction::Pan>())
    {
      setCursor(Qt::OpenHandCursor);

      MouseAction::Pan pan;
      mapView->editorAction.set(pan);
    }

    break;
  }
  case Qt::Key_Z:
    if (e->modifiers() & Qt::CTRL)
    {
      mapView->undo();
    }
    break;
  default:
    e->ignore();
    QVulkanWindow::keyPressEvent(e);
    break;
  }
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

/*
>>>>>>>>>>ContextMenu<<<<<<<<<<<
*/

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

bool VulkanWindow::event(QEvent *event)
{
  // if (!(event->type() == QEvent::UpdateRequest) && !(event->type() == QEvent::MouseMove))
  // {
  //   qDebug() << "[" << QString(debugName.c_str()) << "] " << event->type() << " { " << mapToGlobal(position()) << " }";
  // }

  switch (event->type())
  {
  // case QEvent::MouseMove:
  // {
  //   auto mouseEvent = static_cast<QMouseEvent *>(event);
  //   mouseMoveEvent(mouseEvent);

  //   auto diff = mouseEvent->buttons() ^ mouseState.buttons();
  //   if (diff)
  //   {
  //     if (diff & Qt::MouseButton::LeftButton)
  //     {
  //       bool press = mouseEvent->buttons() & Qt::MouseButton::LeftButton;
  //       auto type = press
  //                       ? QEvent::MouseButtonPress
  //                       : QEvent::MouseButtonRelease;

  //       QMouseEvent e(type,
  //                     mouseEvent->pos(),
  //                     mouseEvent->button(),
  //                     mouseEvent->buttons(),
  //                     mouseEvent->modifiers());

  //       if (press)
  //       {
  //         mapView->mousePressEvent(QtUtil::vmeMouseEvent(&e));
  //       }
  //       else
  //       {
  //         VME_LOG_D("Release from QEvent::MouseMove");
  //         mapView->mouseReleaseEvent(QtUtil::vmeMouseEvent(&e));
  //       }
  //     }
  //     if (diff & Qt::MouseButton::RightButton)
  //     {
  //       bool press = mouseEvent->buttons() & Qt::MouseButton::RightButton;
  //       auto type = press
  //                       ? QEvent::MouseButtonPress
  //                       : QEvent::MouseButtonRelease;

  //       QMouseEvent e(type,
  //                     mouseEvent->pos(),
  //                     mouseEvent->button(),
  //                     mouseEvent->buttons(),
  //                     mouseEvent->modifiers());

  //       if (press)
  //       {
  //         mapView->mousePressEvent(QtUtil::vmeMouseEvent(&e));
  //       }
  //       else
  //       {
  //         mapView->mouseReleaseEvent(QtUtil::vmeMouseEvent(&e));
  //       }
  //     }

  //     mouseState.setButtons(mouseEvent->buttons());
  //   }
  //   return true;
  // }
  // case QEvent::MouseButtonPress:
  // {
  //   auto mouseEvent = static_cast<QMouseEvent *>(event);

  //   if (mouseState.pressed(mouseEvent->button()))
  //   {
  //     return true;
  //   }
  //   mouseState.setPressed(mouseEvent->button());
  //   break;
  // }
  // case QEvent::MouseButtonRelease:
  // {
  //   auto mouseEvent = static_cast<QMouseEvent *>(event);

  //   if (!mouseState.pressed(mouseEvent->button()))
  //   {
  //     return true;
  //   }

  //   VME_LOG_D("Release from QEvent::MouseButtonRelease");
  //   mouseState.setReleased(mouseEvent->button());
  //   break;
  // }
  // case QEvent::MouseButtonDblClick:
  // {
  //   VME_LOG_D("VulkanWindow::MouseButtonDblClick");
  //   break;
  // }
  case QEvent::Leave:
    mapView->setUnderMouse(false);
    break;
  case QEvent::Enter:
    mapView->setUnderMouse(true);
    break;
  case QEvent::ShortcutOverride:
  {
    // Shortcut override event
    QKeyEvent *e = static_cast<QKeyEvent *>(event);
    QKeyEvent keyEvent(QEvent::KeyPress, e->key(), e->modifiers(), e->text(), e->isAutoRepeat(), e->count());
    keyPressEvent(&keyEvent);
    return true;
  }
  default:
    break;
  }

  event->ignore();
  return QVulkanWindow::event(event);
}

void VulkanWindow::updateVulkanInfo()
{
  vulkanInfo.update();
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

//>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>MouseState>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>
// MouseState::MouseState()
//     : _buttons(Qt::MouseButton::NoButton) {}

// bool MouseState::pressed(Qt::MouseButton button) const noexcept
// {
//   return _buttons & button;
// }