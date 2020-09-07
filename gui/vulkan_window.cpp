#include "vulkan_window.h"

#include <QMouseEvent>
#include <QFocusEvent>
#include <QMenu>
#include <QDialog>
#include <QAction>

#include <glm/gtc/type_ptr.hpp>

#include "map_renderer.h"
#include "map_view.h"
#include "../logger.h"

#include "qt/logging.h"

VulkanWindow::VulkanWindow(MapView *mapView)
    : mapView(mapView), contextMenu(nullptr), renderer(nullptr), widget(nullptr)
{
}

void VulkanWindow::lostFocus()
{
  if (contextMenu)
  {
    closeContextMenu();
  }
}

QWidget *VulkanWindow::wrapInWidget()
{
  // QWidget *wrapper = QWidget::createWindowContainer(this);
  // widget = wrapper;
  // return wrapper;
  QWidget *wrapper = QWidget::createWindowContainer(this);
  wrapper->setFocusPolicy(Qt::FocusPolicy::NoFocus);

  widget = wrapper;
  VME_LOG_D(widget);
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
  // VME_LOG_D("");
  // VME_LOG_D(position());
  // VME_LOG_D(e->windowPos());
  // VME_LOG_D(e->pos());
  // VME_LOG_D(e->localPos());
  // VME_LOG_D(geometry());
  // if (!geometry().contains(e->pos()))
  // {
  //   if (contextMenu)
  //   {
  //     closeContextMenu();
  //   }
  //   return;
  // }

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
}

void VulkanWindow::keyPressEvent(QKeyEvent *e)
{
  switch (e->key())
  {
  case Qt::Key_Left:
  case Qt::Key_Right:
  case Qt::Key_Up:
  case Qt::Key_Down:
    break;
  default:
    break;
  }
}

MapView *VulkanWindow::getMapView() const
{
  return mapView;
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
  rect.setX(viewport.offsetX);
  rect.setY(viewport.offsetY);
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
  // Propagate the click event to the map window if appropriate
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
