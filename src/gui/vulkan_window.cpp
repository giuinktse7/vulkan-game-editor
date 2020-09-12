#include "vulkan_window.h"

#include <QMouseEvent>
#include <QFocusEvent>
#include <QMenu>
#include <QDialog>
#include <QAction>

#include <memory>

#include <glm/gtc/type_ptr.hpp>

#include "../map_renderer.h"
#include "../map_view.h"

#include "../logger.h"
#include "../position.h"

#include "../qt/logging.h"

VulkanWindow::VulkanWindow(std::shared_ptr<MapView> mapView)
    : mapView(mapView), scrollAngleBuffer(0)
{
  connect(this, &VulkanWindow::scrollEvent, [=](int scrollDelta) { mapView->zoom(scrollDelta); });
  connect(this, &VulkanWindow::mousePosEvent, [=](util::Point<float> mousePos) {
    int x = static_cast<int>(std::round(mousePos.x()));
    int y = static_cast<int>(std::round(mousePos.y()));

    mapView->mouseMoveEvent(ScreenPosition(x, y));
  });
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
  wrapper->setFocusPolicy(Qt::FocusPolicy::NoFocus);

  widget = wrapper;

  return wrapper;
}

QVulkanWindowRenderer *VulkanWindow::createRenderer()
{
  // Memory deleted by QT when QT closes
  renderer = new MapRenderer(*this);
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
  menu->popup(position);
}

void VulkanWindow::mouseReleaseEvent(QMouseEvent *)
{
}

void VulkanWindow::mouseMoveEvent(QMouseEvent *e)
{
  auto pos = e->windowPos();
  util::Point<float> mousePos(pos.x(), pos.y());
  emit mousePosEvent(mousePos);
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

void VulkanWindow::keyPressEvent(QKeyEvent *e)
{
  int speed = 32;
  switch (e->key())
  {
  case Qt::Key_Left:
    // mapView->panCameraX(-speed);
    emit panEvent(-speed, 0);
    break;
  case Qt::Key_Right:
    // mapView->panCameraX(speed);
    emit panEvent(speed, 0);
    break;
  case Qt::Key_Up:
    // mapView->panCameraY(-speed);
    emit panEvent(0, -speed);
    break;
  case Qt::Key_Down:
    // mapView->panCameraY(speed);
    emit panEvent(0, speed);
    break;
  case Qt::Key_0:
    if (e->modifiers().testFlag(Qt::KeyboardModifier::ControlModifier))
    {
      mapView->resetZoom();
    }
    break;
  default:
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

glm::mat4 VulkanWindow::projectionMatrix()
{
  QMatrix4x4 projection = clipCorrectionMatrix(); // adjust for Vulkan-OpenGL clip space differences
  const QSize sz = swapChainImageSize();
  QRectF rect;
  const Viewport &viewport = mapView->getViewport();
  rect.setX(static_cast<qreal>(viewport.offset.x));
  rect.setY(static_cast<qreal>(viewport.offset.y));
  rect.setWidth(sz.width() * viewport.zoom);
  rect.setHeight(sz.height() * viewport.zoom);
  projection.ortho(rect);

  glm::mat4 data;
  float *ptr = glm::value_ptr(data);
  projection.transposed().copyDataTo(ptr);

  return data;
}

/*
>>>>>>>>>>ContextMenu<<<<<<<<<<<
*/

VulkanWindow::ContextMenu::ContextMenu(VulkanWindow *window, QWidget *widget) : QMenu(widget), window(window)
{
}

bool VulkanWindow::ContextMenu::event(QEvent *e)
{
  if (e->type() == QEvent::Type::MouseButtonPress)
  {
    VME_LOG_D("QEvent::Type::MouseButtonPress");
    // return window->event(e);
  }
  else
  {
  }
  return QWidget::event(e);
}

bool VulkanWindow::ContextMenu::selfClicked(QPoint pos) const
{
  return localGeometry().contains(pos);
}

void VulkanWindow::ContextMenu::mousePressEvent(QMouseEvent *event)
{
  // // Propagate the click event to the map window if appropriate
  if (!selfClicked(event->pos()))
  {
    auto posInWindow = window->mapFromGlobal(event->globalPos());
    VME_LOG_D("posInWindow: " << posInWindow);
    VME_LOG_D("Window geometry: " << window->geometry());
    if (window->localGeometry().contains(posInWindow.x(), posInWindow.y()))
    {
      VME_LOG_D("In window");
      window->mousePressEvent(event);
    }
    else
    {
      event->ignore();
      window->lostFocus();
    }
  }
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
