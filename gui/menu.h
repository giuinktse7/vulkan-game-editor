#pragma once

#include <QWidget>
#include <QWidgetAction>

#include <functional>

#include "../logger.h"

class QString;
class QObject;
class QKeySequence;

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