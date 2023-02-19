#pragma once

#include <variant>

#include "debug.h"
#include "map_copy_buffer.h"
#include "position.h"
#include "signal.h"
#include "util.h"

class Selection;
class Tile;
class Item;
class Brush;

namespace VME
{

    enum MouseButtons : int
    {
        NoButton = 0,
        LeftButton = 1 << 0,
        RightButton = 1 << 1,
        MiddleButton = 1 << 2,
        BackButton = 1 << 3,
        ExtraButton1 = 1 << 4,
        ExtraButton2 = 1 << 5,
        ExtraButton3 = 1 << 6,
        ExtraButton4 = 1 << 7
    };
    using MouseButton = MouseButtons;
    VME_ENUM_OPERATORS(MouseButtons)

    enum ModifierKeys : int
    {
        None = 0,
        Shift = 1 << 0,
        Ctrl = 1 << 1,
        Alt = 1 << 2,
    };
    VME_ENUM_OPERATORS(ModifierKeys)

    struct MouseEvent
    {
        MouseEvent(ScreenPosition pos, MouseButtons buttons, ModifierKeys modifiers)
            : _pos(pos), _buttons(buttons), _modifiers(modifiers) {}
        inline MouseButtons buttons() const noexcept
        {
            return _buttons;
        }

        inline ModifierKeys modifiers() const noexcept
        {
            return _modifiers;
        }

        inline ScreenPosition pos() const noexcept
        {
            return _pos;
        }

      private:
        ScreenPosition _pos;

        MouseButtons _buttons;
        ModifierKeys _modifiers;
    };
} // namespace VME

struct MouseAction
{
    struct None
    {
    };

    struct MoveAction
    {
        std::optional<Position> moveOrigin = {};
        std::optional<Position> moveDelta = {};

        bool isMoving() const;

        void setMoveOrigin(const Position &origin);
        void reset();
    };

    struct MapBrush
    {

        MapBrush(uint32_t serverId);
        MapBrush(Brush *brush);

        void nextVariation();
        void prevVariation();

        Brush *brush;

        /*
            If true, the raw item is currently being drag in an area. Once released,
            each position of the area has an item of serverId added.
        */
        bool area = false;

        /**
         *  If true, this action erases instead of adding
         */
        bool erase = false;

        /**
         * Controls brush variation.
         *  Creatures - rotation
         *  Doodad - alternates
         */
        int variationIndex = 0;
    };

    struct Select : public MoveAction
    {
        void updateMoveDelta(const Selection &selection, const Position &currentPosition);

        bool area = false;
    };

    struct DragDropItem : public MoveAction
    {
        DragDropItem(Tile *tile, Item *item);

        void updateMoveDelta(const Position &currentPosition);

        Tile *tile;
        Item *item;
    };

    struct Pan
    {
        std::optional<WorldPosition> cameraOrigin;
        std::optional<ScreenPosition> mouseOrigin;

        bool active() const;
        void stop();
    };

    struct PasteMapBuffer
    {
        PasteMapBuffer(MapCopyBuffer *buffer);
        MapCopyBuffer *buffer;
    };
};

using MouseAction_t = std::variant<MouseAction::None, MouseAction::MapBrush, MouseAction::Select, MouseAction::Pan, MouseAction::DragDropItem, MouseAction::PasteMapBuffer>;

/**
 * Utility class for sending UI information to a MapView.
 *
 * This is a necessary effect of separating normal code and UI code
 *
 * */
class UIUtils
{
  public:
    virtual ~UIUtils() = default;

    virtual double screenDevicePixelRatio() = 0;
    virtual double windowDevicePixelRatio() = 0;
    virtual ScreenPosition mouseScreenPosInView() = 0;
    virtual VME::ModifierKeys modifiers() const = 0;
    virtual void waitForDraw(std::function<void()> f) = 0;
};

/*
  Contains mouse actions that can occur on a MapView.
*/
class EditorAction
{
  public:
    static EditorAction editorAction;

    inline MouseAction_t &action() noexcept
    {
        return _action;
    };

    inline void setPrevious()
    {
        DEBUG_ASSERT(!_locked, "The action is locked.");

        _action = _previousAction.action;
        _locked = _previousAction.locked;

        _previousAction = {MouseAction::None{}, false};

        actionChanged.fire(_action);
    }

    inline const MouseAction_t &previous() const noexcept
    {
        return _previousAction.action;
    }
    /**
     * @return true if the set was successful.
     */
    bool setIfUnlocked(const MouseAction_t action)
    {
        if (_locked)
            return false;

        set(action);
        return true;
    }

    void set(const MouseAction_t action)
    {
        DEBUG_ASSERT(!_locked, "The action is locked.");
        _previousAction = {_action, _locked};
        _action = action;

        actionChanged.fire(_action);
    }

    void setRawBrush(uint32_t serverId) noexcept;
    void setBrush(Brush *brush) noexcept;

    void reset() noexcept
    {
        unlock();
        set(MouseAction::Select{});
    }

    template <typename T>
    T *as()
    {
        return std::get_if<T>(&_action);
    }

    template <typename T>
    bool is()
    {
        return std::holds_alternative<T>(_action);
    }

    template <auto MemberFunction, typename T>
    void onActionChanged(T *instance)
    {
        actionChanged.connect<MemberFunction>(instance);
    }

    inline void lock() noexcept;
    inline void unlock() noexcept;
    inline bool locked() const noexcept;

  private:
    Nano::Signal<void(const MouseAction_t &)> actionChanged;

    struct MouseActionState
    {
        MouseAction_t action;
        bool locked;
    };

    MouseActionState _previousAction = {MouseAction::Select{}, false};
    MouseAction_t _action = MouseAction::Select{};
    bool _locked = false;
};

inline void EditorAction::lock() noexcept
{
    _locked = true;
}

inline void EditorAction::unlock() noexcept
{
    _locked = false;
}

inline bool EditorAction::locked() const noexcept
{
    return _locked;
}