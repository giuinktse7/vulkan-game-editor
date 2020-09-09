#include "menu.h"

#include <QString>
#include <QKeySequence>
#include <QObject>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenuBar>

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