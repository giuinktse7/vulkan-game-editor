#include "mainwindow.h"

#include <QGridLayout>
#include <QPlainTextEdit>
#include <QMenu>
#include <QTabWidget>
#include <QMenuBar>
#include <QtWidgets>
#include <QContextMenuEvent>
#include <QMouseEvent>
#include <QListView>

#include "vulkan_window.h"

#include "items.h"
#include "time_point.h"
#include "item_list.h"
#include "qt_util.h"
#include "border_layout.h"
#include "main.h"

QLabel *itemImage(uint16_t serverId)
{
    QLabel *container = new QLabel;
    container->setPixmap(QtUtil::itemPixmap(serverId));

    return container;
}

void MainWindow::addMapTab(VulkanWindow &vulkanWindow)
{
    QWidget *wrapper = vulkanWindow.wrapInWidget();

    wrapper->setFocusPolicy(Qt::StrongFocus);
    wrapper->setFocus();

    mapTabs->addTab(wrapper, "untitled.otbm");
}

void MainWindow::experiment2()
{
    createMapTabArea();

    BorderLayout *borderLayout = new BorderLayout;
    rootLayout = borderLayout;

    QMenuBar *menu = createMenuBar();
    rootLayout->setMenuBar(menu);

    QListView *listView = new QListView;
    listView->setItemDelegate(new Delegate(this));

    std::vector<ItemTypeModelItem> data;
    data.push_back(ItemTypeModelItem::fromServerId(2554));
    data.push_back(ItemTypeModelItem::fromServerId(2148));
    data.push_back(ItemTypeModelItem::fromServerId(2555));

    QtItemTypeModel *model = new QtItemTypeModel(listView);
    model->populate(std::move(data));

    listView->setModel(model);
    borderLayout->addWidget(listView, BorderLayout::Position::West);

    textEdit = new QPlainTextEdit;
    textEdit->setReadOnly(false);
    textEdit->setPlainText("100");
    textEdit->setMaximumHeight(80);

    borderLayout->addWidget(mapTabs, BorderLayout::Position::Center);

    setLayout(rootLayout);
}

MainWindow::MainWindow(QWidget *parent) : QWidget(parent)
{
    Qt::WindowFlags flags = this->windowFlags();

    experiment2();
}

void MainWindow::createMapTabArea()
{
    mapTabs = new QTabWidget(this);
    mapTabs->setTabsClosable(true);
    QObject::connect(mapTabs, SIGNAL(tabCloseRequested(int)), this, SLOT(closeMapTab(int)));
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    VME_LOG_D("MainWindow::mousePressEvent");
}

MenuAction::MenuActionWidget::MenuActionWidget(QWidget *parent) : QWidget(parent)
{
    setMouseTracking(true);
}

MenuAction::MenuAction(const QString &text, const QKeySequence &shortcut, QObject *parent)
    : QWidgetAction(parent),
      text(text)
{
    setShortcut(shortcut);
}

MenuAction::MenuAction(const QString &text, QObject *parent)
    : QWidgetAction(parent),
      text(text)
{
}

MenuAction::~MenuAction() {}

QWidget *MenuAction::createWidget(QWidget *parent)
{
    QWidget *widget = new MenuActionWidget(parent);
    widget->setProperty("class", "menu-item");

    QHBoxLayout *layout = new QHBoxLayout(parent);
    layout->setMargin(0);

    QLabel *left = new QLabel(this->text, widget);
    layout->addWidget(left);

    if (!shortcut().isEmpty())
    {
        QLabel *right = new QLabel(this->shortcut().toString(), widget);
        right->setAlignment(Qt::AlignRight);
        layout->addWidget(right);
    }

    widget->setLayout(layout);

    return widget;
}

QMenuBar *MainWindow::createMenuBar()
{
    QMenuBar *menuBar = new QMenuBar;
    QMenu *fileMenu = menuBar->addMenu(tr("File"));

    MenuAction *newMap = new MenuAction(tr("New Map"), Qt::CTRL + Qt::Key_N, this);
    newMap->setShortcut(Qt::CTRL + Qt::Key_N);
    newMap->connect(newMap, &QWidgetAction::triggered, [=] { VME_LOG_D("New Map clicked"); });
    fileMenu->addAction(newMap);

    QMenu *editMenu = menuBar->addMenu(tr("Edit"));

    MenuAction *undo = new MenuAction(tr("Undo"), Qt::CTRL + Qt::Key_Z, this);
    editMenu->addAction(undo);

    MenuAction *redo = new MenuAction(tr("Redo"), Qt::CTRL + Qt::SHIFT + Qt::Key_Z, this);
    editMenu->addAction(redo);

    QAction *reloadStyles = new QAction(tr("Reload styles"), this);
    reloadStyles->connect(reloadStyles, &QAction::triggered, [=] {
        ((MainApplication *)(QApplication::instance()))->loadStyleSheet("default");
    });
    menuBar->addAction(reloadStyles);

    return menuBar;
}

void MainWindow::closeMapTab(int index)
{
    std::cout << "MainWindow::closeMapTab" << std::endl;
    this->mapTabs->widget(index)->deleteLater();
    this->mapTabs->removeTab(index);
}