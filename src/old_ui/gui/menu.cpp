#include "menu.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QKeySequence>
#include <QLabel>
#include <QMenuBar>
#include <QObject>
#include <QString>
#include <QVBoxLayout>

MenuAction::MenuActionWidget::MenuActionWidget(QWidget *parent)
    : QWidget(parent)
{
    setMouseTracking(true);
}

MenuAction::MenuAction(const QString &text, const QKeySequence &shortcut, QObject *parent)
    : QWidgetAction(parent),
      text(text)
{
    if (shortcut != 0)
    {
        setShortcut(shortcut);
    }
}

MenuAction::MenuAction(const QString &text, QObject *parent)
    : QWidgetAction(parent),
      text(text) {}

QWidget *MenuAction::createWidget(QWidget *parent)
{
    QWidget *widget = new MenuActionWidget(parent);
    widget->setProperty("class", "menu-item");

    QHBoxLayout *layout = new QHBoxLayout(parent);
    layout->setContentsMargins(0, 0, 0, 0);

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

MenuSeparator::MenuSeparator(QObject *parent)
    : QWidgetAction(parent) {}

QWidget *MenuSeparator::createWidget(QWidget *parent)
{
    QWidget *widget = new QWidget(parent);

    auto layout = new QVBoxLayout(parent);

    // 1px left & right margin is required, otherwise the widget is not visible... (QT Bug?)
    layout->setContentsMargins(7, 9, 7, 7);
    layout->setSpacing(4);

    QWidget *mainSeparator = new QWidget(parent);
    mainSeparator->setFixedHeight(1);
    mainSeparator->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    mainSeparator->setStyleSheet(QString("background-color: #cccccc;"));
    layout->addWidget(mainSeparator);

    widget->setLayout(layout);

    return widget;
}