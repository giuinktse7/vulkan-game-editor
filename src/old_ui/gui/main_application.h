#pragma once

#include <QApplication>
#include <QObject>
#include <QQuickView>
#include <QVulkanInstance>

#include "mainwindow.h"

class QWindow;
class Qwidget;
class QString;
class WelcomeView;

class MainApplication : public QApplication
{
  public:
    MainApplication(int &argc, char **argv);

    void initializeUI();

    int run();
    void loadStyleSheet(const QString &path);

    void showMainWindow();

    MainWindow mainWindow;

  public slots:
    void onApplicationStateChanged(Qt::ApplicationState state);
    void onFocusWindowChanged(QWindow *window);
    void onFocusWidgetChanged(QWidget *widget);

  private:
    QVulkanInstance vulkanInstance;

    QWindow *focusedWindow = nullptr;
    QWidget *prevWidget = nullptr;
    QWidget *currentWidget = nullptr;

    WelcomeView *welcomeView;

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
