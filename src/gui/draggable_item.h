#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <variant>

#include <QByteArray>
#include <QDebug>
#include <QMimeData>
#include <QPixmap>
#include <QPoint>

#include "../debug.h"
#include "../position.h"
#include "../signal.h"

class QWindow;
class QObject;
class QMouseEvent;
class MapView;
class Tile;
class Item;
struct ContainerItem;
struct Container;

namespace ItemDrag
{
    static constexpr auto DraggableItemFormat = "vulkan-game-editor-mimetype:map-item";

    struct DraggableItem
    {
        enum class Type
        {
            MapItem,
            ContainerItem
        };
        virtual ~DraggableItem() = default;

        virtual Item *item() const = 0;
        virtual const Type type() const noexcept = 0;
        virtual QPixmap pixmap() const = 0;
        Item copy() const;

        bool accepted() const noexcept;

        static bool validType(int value);

        static std::unique_ptr<DraggableItem> deserialize(QByteArray &array);

        QByteArray serialize() const;
        virtual QDataStream &serializeInto(QDataStream &dataStream) const = 0;

      private:
        template <typename T, typename std::enable_if<std::is_base_of<DraggableItem, T>::value>::type * = nullptr>
        static std::unique_ptr<DraggableItem> moveToHeap(std::optional<T> value);

        bool _accepted = false;
    };

    struct MapItem final : DraggableItem
    {
        MapItem();
        MapItem(MapView *mapView, Tile *tile, Item *item);

        const Type type() const noexcept override
        {
            return Type::MapItem;
        }

        Item *item() const override;
        QPixmap pixmap() const override;

        std::shared_ptr<Item> moveFromMap();
        static std::optional<MapItem> fromDataStream(QDataStream &dataStream);

        bool operator==(const MapItem &other) const
        {
            return mapView == other.mapView && tile == other.tile && _item == other._item;
        }

        MapView *mapView;
        Tile *tile;
        Item *_item;

      protected:
        QDataStream &serializeInto(QDataStream &dataStream) const override;
    };

    struct ContainerItemDrag final : DraggableItem
    {
        Item *item() const override;
        QPixmap pixmap() const override;

        static std::optional<ContainerItemDrag> fromDataStream(QDataStream &dataStream);

        const Type type() const noexcept override
        {
            return Type::ContainerItem;
        }

        MapView *mapView;
        Position position;

        uint16_t tileIndex;
        std::vector<uint16_t> containerIndices;

        Container *container();
        Container *container() const;
        Item &draggedItem() const;

      protected:
        QDataStream &serializeInto(QDataStream &dataStream) const override;
    };

    /*
  Defines the available MimeData for a drag & drop operation on a map tab.
*/
    class MimeData : public QMimeData
    {
      public:
        MimeData(MimeData &&other) noexcept;
        MimeData &operator=(MimeData &&other) noexcept;

        template <typename T, typename std::enable_if<std::is_base_of<DraggableItem, T>::value>::type * = nullptr>
        static MimeData create(T &&t)
        {
            auto result = MimeData(std::make_unique<T>(std::move(t)));
            return result;
        }

        bool hasFormat(const QString &mimeType) const override;
        QStringList formats() const override;

        QVariant retrieveData(const QString &mimeType, QMetaType type) const override;

        std::unique_ptr<DraggableItem> draggableItem;

      private:
        MimeData(std::unique_ptr<DraggableItem> &&draggableItem);
    };

    class DragOperation
    {
      private:
        using Source = MapView *;

      public:
        enum class DropResult
        {
            Accepted,
            Rejected,
            NoTarget
        };

        DragOperation(DragOperation &&other) noexcept;
        DragOperation &operator=(DragOperation &&other) noexcept;

        template <typename T, typename std::enable_if<std::is_base_of<DraggableItem, T>::value>::type * = nullptr>
        static DragOperation create(T &&t, Source source, QWindow *parent)
        {
            return DragOperation(MimeData::create(std::move(t)), source, parent);
        }

        void setRenderCondition(std::function<bool()> f);

        void start();
        bool isDragging() const;
        bool mouseMoveEvent(QMouseEvent *event);
        bool sendDropEvent(QMouseEvent *event);
        QObject *hoveredObject() const;
        Source source() const noexcept;

        template <auto MemberFunction, typename T>
        void onDragFinished(T *instance);

        ItemDrag::MimeData mimeData;

      private:
        DragOperation(MimeData &&mimeData, Source source, QWindow *parent);

        void sendDragEnterEvent(QObject *object, QPoint position);
        void sendDragEnterEvent(QObject *object, QPoint position, QMouseEvent *event);
        void sendDragLeaveEvent(QObject *object, QPoint position, QMouseEvent *event);
        void sendDragMoveEvent(QObject *object, QPoint position, QMouseEvent *event);

        void setHoveredObject(QObject *object);

        void showCursor();
        void hideCursor();

        QWindow *_parent;
        QObject *_hoveredObject;
        Source _source;

        QPixmap pixmap;

        std::function<bool()> shouldRender = [] { return true; };

        bool renderingCursor;

        Nano::Signal<void(DropResult)> dragFinished;
    };

} // namespace ItemDrag

inline QDebug operator<<(QDebug &dbg, const ItemDrag::MapItem &mapItem)
{
    dbg.nospace() << "MapItem { mapView: " << mapItem.mapView << ", tile: " << mapItem.tile << ", item:" << mapItem._item << "}";
    return dbg.maybeSpace();
}

QDataStream &operator<<(QDataStream &, const ItemDrag::MapItem &);
QDataStream &operator>>(QDataStream &, ItemDrag::MapItem &);

QDataStream &operator<<(QDataStream &, const ItemDrag::ContainerItemDrag &);
QDataStream &operator>>(QDataStream &, ItemDrag::ContainerItemDrag &);

QDataStream &operator<<(QDataStream &, const ItemDrag::DraggableItem &);
QDataStream &operator>>(QDataStream &, ItemDrag::DraggableItem &);

template <typename T, typename std::enable_if<std::is_base_of<ItemDrag::DraggableItem, T>::value>::type *>
std::unique_ptr<ItemDrag::DraggableItem> ItemDrag::DraggableItem::moveToHeap(std::optional<T> value)
{
    return value ? std::make_unique<T>(value.value()) : std::unique_ptr<DraggableItem>{};
}

template <auto MemberFunction, typename T>
void ItemDrag::DragOperation::onDragFinished(T *instance)
{
    dragFinished.connect<MemberFunction>(instance);
}

// Q_DECLARE_METATYPE(DragOperation::MapItem);
// Q_DECLARE_METATYPE(DragOperation::ContainerItem);