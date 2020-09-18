#pragma once

#include <QWidget>
#include <QWidgetAction>
#include <QString>

QT_BEGIN_NAMESPACE
class QPlainTextEdit;
class QLayout;
class QLabel;
class QTabWidget;
class QPushButton;
class BorderLayout;
QT_END_NAMESPACE

class MapTabWidget;

#include "vulkan_window.h"

#define QT_MANAGED_POINTER(cls, ...) new cls(__VA_ARGS__);

class MainWindow : public QWidget
{
  Q_OBJECT

public:
  MainWindow(QWidget *parent = nullptr);

  void setVulkanInstance(QVulkanInstance *instance);

protected:
  void mousePressEvent(QMouseEvent *event) override;

private:
  // UI
  BorderLayout *rootLayout;
  MapTabWidget *mapTabs;

  QLabel *positionStatus;

  void experimentLayout();
  void initializeUI();

  QMenuBar *createMenuBar();
  void createMapTabArea();


  QVulkanInstance *vulkanInstance;

  // void updatePositionText();
};
