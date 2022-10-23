#pragma once

#include "common/editor_action.h"

class QmlUIUtils : public UIUtils
{
  public:
    QmlUIUtils() {}
    ScreenPosition mouseScreenPosInView() override;

    double screenDevicePixelRatio() override;
    double windowDevicePixelRatio() override;

    VME::ModifierKeys modifiers() const override;

    void waitForDraw(std::function<void()> f) override;
};
