#pragma once

#include <QWidget>

QT_BEGIN_NAMESPACE
class QPlainTextEdit;
class QLayout;
class QTabWidget;
QT_END_NAMESPACE

#include "vulkan_window.h"

class MainWindow : public QWidget
{
    Q_OBJECT

public:
    MainWindow();

    void addMapTab(VulkanWindow &vulkanWindow);

protected:
    void mousePressEvent(QMouseEvent *event) override;

private slots:
    void closeMapTab(int index);

private:
    // UI
    QPlainTextEdit *textEdit;
    QLayout *rootLayout;
    QTabWidget *mapTabs;

    void experimentLayout();
    void experiment2();

    QMenuBar *createMenuBar();
    void createMapTabArea();
};
