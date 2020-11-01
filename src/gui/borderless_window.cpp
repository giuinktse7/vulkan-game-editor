#include "borderless_window.h"

#include <QDrag>
#include <QHBoxLayout>
#include <QLabel>
#include <QMimeData>
#include <QMouseEvent>
#include <QPushButton>
#include <QStyle>
#include <QVBoxLayout>

#include "border_layout.h"

#include "../qt/logging.h"

BorderlessMainWindow::BorderlessMainWindow(QWidget *parent) : QWidget(parent, Qt::CustomizeWindowHint)
{
  setObjectName("borderlessMainWindow");
  setWindowFlags(Qt::FramelessWindowHint);

  VME_LOG_D("BorderlessMainWindow: " << this);

  // this->mainWindow = mainWindow;
  // setWindowTitle(mainWindow->windowTitle());

  // BorderLayout *mainLayout = new BorderLayout(0);
  mainLayout = new QVBoxLayout();
  // QHBoxLayout *horizontalLayout = new QHBoxLayout();
  // horizontalLayout->setSpacing(0);
  // horizontalLayout->setMargin(0);

  // titleBar = new QWidget(this);
  // titleBar->setObjectName("titlebarWidget");
  // titleBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

  // minimizeButton = new QPushButton(titleBar);
  // connect(minimizeButton, &QPushButton::clicked, this, &BorderlessMainWindow::slotMinimized);

  // restoreButton = new QPushButton(titleBar);
  // restoreButton->setVisible(false);
  // connect(restoreButton, &QPushButton::clicked, this, &BorderlessMainWindow::slotRestored);

  // maximizeButton = new QPushButton(titleBar);
  // connect(maximizeButton, &QPushButton::clicked, this, &BorderlessMainWindow::slotMaximized);

  // closeButton = new QPushButton(titleBar);
  // connect(closeButton, &QPushButton::clicked, this, &BorderlessMainWindow::slotClosed);

  // title = new QLabel(titleBar);
  // title->setText(windowTitle());

  // titleBar->setLayout(horizontalLayout);
  // titleBar->setStyleSheet("{ background: red; }");

  // horizontalLayout->addWidget(title);
  // horizontalLayout->addStretch(1);
  // horizontalLayout->addWidget(minimizeButton);
  // horizontalLayout->addWidget(restoreButton);
  // horizontalLayout->addWidget(maximizeButton);
  // horizontalLayout->addWidget(closeButton);

  // mainLayout->addWidget(titleBar, BorderLayout::Position::North);
  // mainLayout->addWidget(mainWindow, BorderLayout::Position::Center);
  // mainLayout->addWidget(titleBar);
  // mainLayout->addWidget(mainWindow);

  QLabel *label = new QLabel(this);
  label->setText("Left panel");
  // mainLayout->addWidget(label, BorderLayout::Position::West);
  mainLayout->addWidget(label);

  setLayout(mainLayout);
}

void BorderlessMainWindow::addWidget(QWidget *widget)
{
  mainLayout->addWidget(widget);
}

void BorderlessMainWindow::mousePressEvent(QMouseEvent *event)
{
  if (event->button() == Qt::LeftButton)
  {
    int width = this->size().width();
    int height = this->size().height();
    QPoint pos = event->pos();
    int x = pos.x();
    int y = pos.y();

    int allowance = 2;

#define __BETWEEN(x, a, b) (a <= x && x <= b)
    bool leftDrag = __BETWEEN(x, 0, allowance) && __BETWEEN(y, 0, height);
    bool rightDrag = __BETWEEN(x, width - allowance, width) && __BETWEEN(y, 0, height);
    bool topDrag = __BETWEEN(x, 0, width) && __BETWEEN(y, 0, allowance);
    bool bottomDrag = __BETWEEN(x, 0, width) && __BETWEEN(y, height - allowance, height);
#undef __BETWEEN

    bool startDrag = leftDrag || rightDrag || topDrag || bottomDrag;

    if (startDrag)
    {
      VME_LOG_D("Start drag");
      QWindow *topWindow = app->topLevelWindows().first();
      for (auto window : app->topLevelWindows())
      {
        VME_LOG_D("Top-level window: " << window);
      }
      VME_LOG_D("windowHandle: " << window()->windowHandle());
      topWindow->requestActivate();
      topWindow->startSystemResize(Qt::RightEdge);
      // QDrag *drag = new QDrag(this);
      // QMimeData *mimeData = new QMimeData;

      // drag->setMimeData(mimeData);
      // Qt::DropAction dropAction = drag->exec();
    }
  }

  // if (!titleBar->underMouse() && !title->underMouse())
  //   return;

  // if (event->button() == Qt::LeftButton)
  // {
  //   moving = true;
  //   lastMousePos = event->pos();
  // }
}

void BorderlessMainWindow::mouseMoveEvent(QMouseEvent *event)
{
  // if (!titleBar->underMouse() && !title->underMouse())
  //   return;

  // if (event->buttons().testFlag(Qt::LeftButton) && moving)
  // {
  //   this->move(this->pos() + (event->pos() - lastMousePos));
  // }
}

void BorderlessMainWindow::mouseReleaseEvent(QMouseEvent *event)
{
  // if (!titleBar->underMouse() && !title->underMouse())
  //   return;

  // if (event->button() == Qt::LeftButton)
  // {
  //   moving = false;
  // }
}

void BorderlessMainWindow::mouseDoubleClickEvent(QMouseEvent *event)
{
  // Q_UNUSED(event);
  // if (!titleBar->underMouse() && !title->underMouse())
  //   return;

  // maximized = !maximized;
  // if (maximized)
  // {
  //   slotMaximized();
  // }
  // else
  // {
  //   slotRestored();
  // }
}

void BorderlessMainWindow::slotMinimized(bool)
{
  setWindowState(Qt::WindowMinimized);
}
void BorderlessMainWindow::slotRestored(bool)
{
  // restoreButton->setVisible(false);
  // maximizeButton->setVisible(true);
  // setWindowState(Qt::WindowNoState);
  // setStyleSheet("#borderlessMainWindow{border:1px solid palette(highlight);}");
}
void BorderlessMainWindow::slotMaximized(bool)
{
  // restoreButton->setVisible(true);
  // maximizeButton->setVisible(false);
  // setWindowState(Qt::WindowMaximized);
  // setStyleSheet("#borderlessMainWindow{border:1px solid palette(base);}");
}
void BorderlessMainWindow::slotClosed(bool)
{
  close();
}