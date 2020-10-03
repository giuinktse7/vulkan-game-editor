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
#include "qt_util.h"

#include "gui.h"

VulkanWindow::VulkanWindow(std::unique_ptr<MapView> mapView, MapViewMouseAction &mapViewMouseAction)
    : QVulkanWindow(nullptr),
      mapViewMouseAction(mapViewMouseAction),
      mapView(std::move(mapView)),
      scrollAngleBuffer(0)
{
  connect(this, &VulkanWindow::scrollEvent, [=](int scrollDelta) { this->mapView->zoom(scrollDelta); });
  connect(this, &VulkanWindow::mousePosChanged, [=](util::Point<float> mousePos) {
    int x = static_cast<int>(std::round(mousePos.x()));
    int y = static_cast<int>(std::round(mousePos.y()));

    this->mapView->mouseMoveEvent(ScreenPosition(x, y));
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
  QtUtil::associateWithMapView(*wrapper, mapView.get());

  widget = wrapper;

  return wrapper;
}

QVulkanWindowRenderer *VulkanWindow::createRenderer()
{
  if (!renderer)
  {
    // Memory deleted by QT when QT closes
    renderer = new MapRenderer(*this);
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
  auto pos = event->windowPos();
  util::Point<float> mousePos(pos.x(), pos.y());
  emit mousePosChanged(mousePos);

  mapView->mouseMoveEvent(QtUtil::vmeMouseEvent(event));

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

bool VulkanWindow::event(QEvent *ev)
{

  // qDebug() << "[" << QString(debugName.c_str()) << "] " << ev->type() << " { " << mapToGlobal(position()) << " }";

  switch (ev->type())
  {
  case QEvent::Leave:
    showPreviewCursor = false;
    break;
  case QEvent::Enter:
    showPreviewCursor = true;
    {
      auto pos = mapFromGlobal(QCursor::pos());
      mapView->setMousePos(ScreenPosition(pos.x(), pos.y()));
    }
    break;
  default:
    break;
  }

  ev->ignore();
  return QVulkanWindow::event(ev);
}

void VulkanWindow::updateVulkanInfo()
{
  vulkanInfo.window = this;
  vulkanInfo.df = vulkanInstance()->deviceFunctions(device());
}

VulkanInfoFromWindow::VulkanInfoFromWindow() : window(nullptr)
{
}

VulkanInfoFromWindow::VulkanInfoFromWindow(VulkanWindow *window)
    : window(window)
{
  df = window->vulkanInstance()->deviceFunctions(window->device());
}
