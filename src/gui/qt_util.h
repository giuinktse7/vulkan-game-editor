#pragma once

#include <QApplication>
#include <QDataStream>
#include <QMouseEvent>
#include <QPixmap>
#include <QQuickImageProvider>
#include <QString>
#include <QWidget>

#include <memory>
#include <optional>
#include <unordered_map>

#include "../editor_action.h"

#include "../creature.h"
#include "../position.h"
#include "../qt/enum_conversion.h"
#include "../util.h"

class MainApplication;

class QWheelEvent;
struct TextureInfo;
class Item;
class MapView;
class VulkanWindow;
struct TextureAtlas;

namespace
{
    constexpr int QtMinimumWheelDelta = 8;
    constexpr int DefaultMinimumAngleDelta = 15;
} // namespace

namespace QtUtil
{
    /**
     * Makes it possible to mix connect syntax like:
     * QmlBind::connect(countSpinbox, SIGNAL(valueChanged()), [=]{...});
     */
    class QmlBind : public QObject
    {
        Q_OBJECT

        using F = std::function<void()>;

      public:
        Q_SLOT void call()
        {
            f();
        }

        static QMetaObject::Connection connect(QObject *sender, const char *signal, F &&f);

      private:
        QmlBind(F &&f, QObject *parent = {});

        F f;
    };

    class QtUiUtils : public UIUtils
    {
      public:
        QtUiUtils(VulkanWindow *window)
            : window(window) {}
        ScreenPosition mouseScreenPosInView() override;

        double screenDevicePixelRatio() override;
        double windowDevicePixelRatio() override;

        VME::ModifierKeys modifiers() const override;

        void waitForDraw(std::function<void()> f) override;

      private:
        VulkanWindow *window;
    };

    namespace PropertyName
    {
        constexpr const char *MapView = "vme-mapview";
        constexpr const char *VulkanWindow = "vme-vulkan-window";
    } // namespace PropertyName

    template <typename T, typename std::enable_if<std::is_pointer<T>::value>::type * = nullptr>
    T readPointer(QDataStream &dataStream)
    {
        // static_assert(std::is_pointer<T>::value, "Expected a pointer");

        util::PointerAddress pointer;
        dataStream >> pointer;
        return (T)pointer;
    }

    template <typename T>
    inline QVariant wrapPointerInQVariant(T *pointer)
    {
        return QVariant::fromValue(static_cast<void *>(pointer));
    }

    inline void setMapView(QWidget &widget, MapView *mapView)
    {
        widget.setProperty(PropertyName::MapView, wrapPointerInQVariant(mapView));
    }

    inline void setVulkanWindow(QWidget &widget, VulkanWindow *vulkanWindow)
    {
        widget.setProperty(PropertyName::VulkanWindow, wrapPointerInQVariant(vulkanWindow));
    }

    MapView *associatedMapView(QWidget *widget);
    VulkanWindow *associatedVulkanWindow(QWidget *widget);

    struct ScrollState
    {
        ScrollState() {}
        ScrollState(int minimumAngleDelta)
            : minRotationDelta(minimumAngleDelta * QtMinimumWheelDelta) {}

        int amountBuffer = 0;

        std::optional<int> scroll(QWheelEvent *event);

      private:
        /*
      The minimum rotation amount for a scroll to be registered, in eighths of a degree.
      For example, 120 MinRotationAmount = (120 / 8) = 15 degrees of rotation.
    */
        int minRotationDelta = DefaultMinimumAngleDelta * QtMinimumWheelDelta;
    };

    inline VME::MouseEvent vmeMouseEvent(QMouseEvent *event)
    {
        VME::MouseButtons buttons = enum_conversion::vmeButtons(event->buttons());
        VME::ModifierKeys modifiers = enum_conversion::vmeModifierKeys(event->modifiers());

        ScreenPosition pos(event->pos().x(), event->pos().y());
        return VME::MouseEvent(pos, buttons, modifiers);
    }

    QString getItemTypeResourcePath(uint32_t serverId, uint8_t subtype = 0);
    QString getCreatureTypeResourcePath(const CreatureType &creatureType, Direction direction = Direction::South);

    MainApplication *qtApp();

    QString resourcePath(Brush *brush);

    class EventFilter : public QObject
    {
      public:
        EventFilter(QObject *parent)
            : QObject(parent) {}

      protected:
        virtual bool eventFilter(QObject *obj, QEvent *event) = 0;
    };
} // namespace QtUtil

class QmlApplicationContext : public QObject
{
    Q_OBJECT
  public:
    explicit QmlApplicationContext(QObject *parent = 0)
        : QObject(parent) {}
    Q_INVOKABLE void setCursor(Qt::CursorShape cursor)
    {
        QApplication::setOverrideCursor(cursor);
    }

    Q_INVOKABLE void resetCursor()
    {
        QApplication::restoreOverrideCursor();
    }
};

template <typename T>
inline QString toQString(T value)
{
    std::ostringstream s;
    s << value;
    return QString::fromStdString(s.str());
}

inline QDataStream &operator<<(QDataStream &dataStream, const Position &position)
{
    dataStream << position.x;
    dataStream << position.y;
    dataStream << position.z;

    return dataStream;
}

inline QDataStream &operator>>(QDataStream &dataStream, Position &position)
{
    dataStream >> position.x;
    dataStream >> position.y;
    dataStream >> position.z;

    return dataStream;
}