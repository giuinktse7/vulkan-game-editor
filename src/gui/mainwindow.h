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
  BorderLayout *rootLayout;
  QTabWidget *mapTabs;

  QLabel *positionStatus;

  void experimentLayout();
  void initializeUI();

  QMenuBar *createMenuBar();
  void createMapTabArea();

  // void updatePositionText();
};
