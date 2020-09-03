#pragma once

#include <QWidget>

QT_BEGIN_NAMESPACE
class QPlainTextEdit;
class QLayout;
class QTabWidget;
QT_END_NAMESPACE

class VulkanWindow;

class MainWindow : public QWidget
{
    Q_OBJECT

public:
    MainWindow(VulkanWindow *vulkanWindow);

private slots:
    void closeMapTab(int index);

private:
    // UI
    QPlainTextEdit *textEdit;
    QLayout *rootLayout;
    QTabWidget *mapTabs;

    void createMenuBar();
};
