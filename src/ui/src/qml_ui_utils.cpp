#include "qml_ui_utils.h"
#include "enum_conversion.h"

#include <QGuiApplication>

ScreenPosition QmlUIUtils::mouseScreenPosInView()
{
    ABORT_PROGRAM("Not implemented");
    return ScreenPosition(0, 0);
}

double QmlUIUtils::screenDevicePixelRatio()
{
    return 1.0;
}

double QmlUIUtils::windowDevicePixelRatio()
{
    return 1.0;
}

VME::ModifierKeys QmlUIUtils::modifiers() const
{
    return enum_conversion::vmeModifierKeys(QGuiApplication::keyboardModifiers());
}

void QmlUIUtils::waitForDraw(std::function<void()> f)
{
    f();
}