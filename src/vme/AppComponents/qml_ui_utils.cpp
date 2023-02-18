#include "qml_ui_utils.h"

#include <QGuiApplication>
#include <QtQuick/QQuickWindow>

#include "enum_conversion.h"

QmlUIUtils::QmlUIUtils(QQuickWindow *window)
    : window(window) {}

ScreenPosition QmlUIUtils::mouseScreenPosInView()
{
    auto pos = window->mapFromGlobal(QCursor::pos());
    return ScreenPosition(pos.x(), pos.y());
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