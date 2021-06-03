#pragma once

#include <QApplication>
#include <QVulkanInstance>

#include "mainwindow.h"

class QWindow;
class Qwidget;
class QString;

class MainApplication : public QApplication
{
  public:
    MainApplication(int &argc, char **argv);

    void initializeUI();

    int run();

    MainWindow mainWindow;

  public slots:
    void onApplicationStateChanged(Qt::ApplicationState state);
    void onFocusWindowChanged(QWindow *window);
    void onFocusWidgetChanged(QWidget *widget);

    void loadStyleSheet(const QString &path);

  private:
    QVulkanInstance vulkanInstance;

    QWindow *focusedWindow = nullptr;
    QWidget *prevWidget = nullptr;
    QWidget *currentWidget = nullptr;

    // QWindow *vulkanWindow = nullptr;

    class EventFilter : public QObject
    {
      public:
        EventFilter(MainApplication *mainApplication);

        bool eventFilter(QObject *obj, QEvent *event) override;

      private:
        MainApplication *mainApplication;
    };
};
