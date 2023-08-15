#pragma once

#include <QEvent>

#include "core/editor_action.h"

enum CustomQEvent
{
    RenderMapFinished = QEvent::User + 1,
};

class QQuickItem;

class QmlUIUtils : public UIUtils
{
  public:
    QmlUIUtils(QQuickItem *qItem);
    ScreenPosition mouseScreenPosInView() override;

    double screenDevicePixelRatio() override;
    double windowDevicePixelRatio() override;

    VME::ModifierKeys modifiers() const override;

    void waitForDraw(std::function<void()> f) override;

  private:
    QQuickItem *_item;
};
