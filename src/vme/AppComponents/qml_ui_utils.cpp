#include "qml_ui_utils.h"

#include <QCursor>
#include <QGuiApplication>
#include <QQuickItem>

#include "enum_conversion.h"

QmlUIUtils::QmlUIUtils(QQuickItem *item)
    : _item(item) {}

ScreenPosition QmlUIUtils::mouseScreenPosInView()
{
    auto pos = _item->mapFromGlobal(QCursor::pos());
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