#include "enum_conversion.h"

#include "common/editor_action.h"

using namespace enum_conversion::detail;

namespace
{
    // Table for converting Qt::KeyboardModifiers to VME::ModifierKeys
    using ModifierKeysConversion = BitFlagConversionTable<
        Qt::KeyboardModifier,
        VME::ModifierKeys,
        util::msb(Qt::KeyboardModifier::GroupSwitchModifier)>;
    constexpr ModifierKeysConversion createModifierKeyTable()
    {
        using qtKey = Qt::KeyboardModifier;
        using vmeKey = VME::ModifierKeys;
        ModifierKeysConversion table;
        table.add(qtKey::ShiftModifier, vmeKey::Shift);
        table.add(qtKey::ControlModifier, vmeKey::Ctrl);
        table.add(qtKey::AltModifier, vmeKey::Alt);

        return table;
    }
    constexpr ModifierKeysConversion ModifierKeysConversionTable = createModifierKeyTable();

    // Table for converting Qt::MouseButtons to VME::MouseButtons
    using MouseButtonConversion = BitFlagConversionTable<
        Qt::MouseButton,
        VME::MouseButtons,
        util::msb(Qt::MouseButton::MaxMouseButton)>;
    constexpr MouseButtonConversion createMouseButtonTable()
    {
        using qtButton = Qt::MouseButton;
        using vmeButton = VME::MouseButton;
        MouseButtonConversion table;
        table.add(qtButton::LeftButton, vmeButton::LeftButton);
        table.add(qtButton::RightButton, vmeButton::RightButton);
        table.add(qtButton::MiddleButton, vmeButton::MiddleButton);
        table.add(qtButton::BackButton, vmeButton::BackButton);
        table.add(qtButton::ExtraButton1, vmeButton::ExtraButton1);
        table.add(qtButton::ExtraButton2, vmeButton::ExtraButton2);
        table.add(qtButton::ExtraButton3, vmeButton::ExtraButton3);
        table.add(qtButton::ExtraButton4, vmeButton::ExtraButton4);

        return table;
    }
    constexpr MouseButtonConversion MouseButtonsConversionTable = createMouseButtonTable();
} // namespace

VME::ModifierKeys enum_conversion::vmeModifierKeys(Qt::KeyboardModifiers modifiers)
{
    auto enumValue = static_cast<Qt::KeyboardModifier>(static_cast<Qt::KeyboardModifiers::Int>(modifiers));
    return ModifierKeysConversionTable.from(enumValue);
}

VME::MouseButtons enum_conversion::vmeButtons(Qt::MouseButtons buttons)
{
    auto enumValue = static_cast<Qt::MouseButton>(static_cast<Qt::MouseButtons::Int>(buttons));
    return MouseButtonsConversionTable.from(enumValue);
}