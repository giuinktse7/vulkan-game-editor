#pragma once

#include "gui/main_application.h"
#include "mainwindow.h"
#include <QWidget>

class QLabel;
class QPushButton;

class BorderlessMainWindow : public QWidget
{
    Q_OBJECT
  public:
    explicit BorderlessMainWindow(QWidget *parent);

    void addWidget(QWidget *widget);

    QLayout *mainLayout;

    MainApplication *app;

  protected:
    void
    mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);
  private slots:
    void slotMinimized(bool = false);
    void slotRestored(bool = false);
    void slotMaximized(bool = false);
    void slotClosed(bool = false);

  private:
    // QWidget *mainWindow;
    // QWidget *titleBar;
    // QLabel *title;
    // QPushButton *minimizeButton;
    // QPushButton *restoreButton;
    // QPushButton *maximizeButton;
    // QPushButton *closeButton;
    QPoint lastMousePos;
    bool moving;
    bool maximized;
};