#pragma once

#include "core/editor_action.h"

class QQuickWindow;

class QmlUIUtils : public UIUtils
{
  public:
    QmlUIUtils(QQuickWindow *window);
    ScreenPosition mouseScreenPosInView() override;

    double screenDevicePixelRatio() override;
    double windowDevicePixelRatio() override;

    VME::ModifierKeys modifiers() const override;

    void waitForDraw(std::function<void()> f) override;

  private:
    QQuickWindow *window;
};
