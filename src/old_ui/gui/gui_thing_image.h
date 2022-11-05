#pragma once

#include <memory>

#include <QQuickImageProvider>
#include <QRect>

#include "../graphics/texture.h"
#include "../util.h"

class QImage;
class QPixmap;
class QString;
class QSize;

class ItemType;
class CreatureType;
struct Position;
class Item;
enum class Direction;
struct TextureInfo;
struct TextureWindow;

struct QtTextureArea
{
    QtTextureArea(const TextureWindow &window, const Texture &texture);
    QtTextureArea(QRect rect, QImage *image);

    QRect rect;
    QImage *image;
};

struct GUIImageCache
{

    static void initialize();
    static const QPixmap &get(uint32_t serverId);
    static void cachePixmapForServerId(uint32_t serverId);
    static QImage *getOrCreateQImageForTexture(const Texture &texture);
    static QPixmap blackSquarePixmap();

    static vme_unordered_map<uint32_t, QPixmap> serverIdToPixmap;
    static vme_unordered_map<uint32_t, std::unique_ptr<QImage>> textureIdToQImage;
    static std::unique_ptr<QPixmap> blackSquare;
};

class GUIThingImage
{
  public:
    static QImage getCreatureTypeImage(const CreatureType &creatureType, Direction direction);
    static QImage getItemTypeImage(const ItemType &itemType, uint8_t subtype = 0);
    static QImage getItemTypeImage(uint32_t serverId, uint8_t subtype = 0);

    static QPixmap itemPixmap(uint32_t serverId, uint8_t subtype = 0);
    static QPixmap itemPixmap(const Position &pos, const Item &item);

    static QPixmap creaturePixmap(uint32_t looktype, Direction direction);
    static QPixmap creaturePixmap(const CreatureType *creatureType, Direction direction);

  private:
    static QPixmap thingPixmap(const TextureInfo &info);
    static QPixmap thingPixmap(const TextureWindow &textureWindow, const Texture &texture, uint16_t spriteWidth, uint16_t spriteHeight);

    static QtTextureArea getTextureArea();
};

class ItemTypeImageProvider : public QQuickImageProvider
{
  public:
    ItemTypeImageProvider()
        : QQuickImageProvider(QQuickImageProvider::Pixmap) {}

    QPixmap requestPixmap(const QString &id, QSize *size, const QSize &requestedSize) override;
};

class CreatureImageProvider : public QQuickImageProvider
{
  public:
    CreatureImageProvider()
        : QQuickImageProvider(QQuickImageProvider::Pixmap) {}

    QPixmap requestPixmap(const QString &id, QSize *size, const QSize &requestedSize) override;
};