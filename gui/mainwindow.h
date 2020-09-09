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
    QPlainTextEdit *textEdit;
    QLayout *rootLayout;
    QTabWidget *mapTabs;

    void experimentLayout();
    void experiment2();

    QMenuBar *createMenuBar();
    void createMapTabArea();
};

class MenuAction : public QWidgetAction
{
    Q_OBJECT
public:
    class MenuActionWidget : public QWidget
    {
    public:
        MenuActionWidget(QWidget *parent = nullptr);
    };

    MenuAction(const QString &text, QObject *parent = nullptr);
    MenuAction(const QString &text, const QKeySequence &shortcut, QObject *parent = nullptr);
    ~MenuAction();
    QWidget *createWidget(QWidget *parent);

private:
    QString text;
};