#pragma once

#include <QtQuick/QQuickItem>

#include "core/editor_action.h"

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
