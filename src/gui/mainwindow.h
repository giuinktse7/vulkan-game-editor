#pragma once

#include <QWidget>
#include <QWidgetAction>
#include <QString>

QT_BEGIN_NAMESPACE
class QPlainTextEdit;
class QLayout;
class QTabWidget;
class QPushButton;
QT_END_NAMESPACE

#include "vulkan_window.h"

class MainWindow : public QWidget
{
  Q_OBJECT

public:
  MainWindow(QWidget *parent = nullptr);

  void addMapTab(VulkanWindow &vulkanWindow);

protected:
  void mousePressEvent(QMouseEvent *event) override;

public slots:
  void closeMapTab(int index);

private:
  // UI
  QLayout *rootLayout;
  QTabWidget *mapTabs;

  void experimentLayout();
  void initializeUI();

  QMenuBar *createMenuBar();
  void createMapTabArea();
};
