#include "tileset_model.h"
#include "ui_resources.h"

#include "core/brushes/border_brush.h"
#include "core/brushes/brush.h"
#include "core/brushes/creature_brush.h"
#include "core/brushes/doodad_brush.h"
#include "core/brushes/ground_brush.h"
#include "core/brushes/mountain_brush.h"
#include "core/brushes/raw_brush.h"
#include "core/brushes/wall_brush.h"
#include "core/tileset.h"
#include "gui_thing_image.h"

TileSetModel::TileSetModel()
    : _tileset(std::make_shared<Tileset>("all", "All"))
{
}

TileSetModel::TileSetModel(std::shared_ptr<Tileset> tileset)
    : _tileset(tileset) {}

int TileSetModel::rowCount(const QModelIndex &) const
{
    return static_cast<int>(std::ceil(_tileset->size() / columnCount()));
}

int TileSetModel::columnCount(const QModelIndex &) const
{
    return 1;
}

void setTileset(std::shared_ptr<Tileset> &&tileset);

QVariant TileSetModel::data(const QModelIndex &modelIndex, int role) const
{
    switch (role)
    {
        case Qt::DisplayRole:
            return QString("%1, %2").arg(modelIndex.column()).arg(modelIndex.row());
        case Roles::ImageUriRole:
        {
            uint32_t index = modelIndex.row() * columnCount() + modelIndex.column();
            Brush *thingBrush = _tileset->get(index);

            switch (thingBrush->type())
            {
                case BrushType::Raw:
                {
                    auto brush = static_cast<RawBrush *>(thingBrush);
                    uint32_t serverId = brush->serverId();
                    return UIResource::getItemPixmapString(serverId, 0);
                }
                case BrushType::Ground:
                {
                    auto brush = static_cast<GroundBrush *>(thingBrush);
                    uint32_t serverId = brush->iconServerId();
                    return UIResource::getItemPixmapString(serverId, 0);
                }
                case BrushType::Border:
                {
                    auto brush = static_cast<BorderBrush *>(thingBrush);
                    uint32_t serverId = brush->iconServerId();
                    return UIResource::getItemPixmapString(serverId, 0);
                    break;
                }
                case BrushType::Wall:
                {
                    auto brush = static_cast<WallBrush *>(thingBrush);
                    uint32_t serverId = brush->iconServerId();
                    return UIResource::getItemPixmapString(serverId, 0);
                    break;
                }
                case BrushType::Mountain:
                {
                    auto brush = static_cast<MountainBrush *>(thingBrush);
                    uint32_t serverId = brush->iconServerId();
                    return UIResource::getItemPixmapString(serverId, 0);
                    break;
                }
                case BrushType::Doodad:
                {
                    auto brush = static_cast<DoodadBrush *>(thingBrush);
                    uint32_t serverId = brush->iconServerId();
                    return UIResource::getItemPixmapString(serverId, 0);
                    break;
                }
                case BrushType::Creature:
                {
                    auto brush = static_cast<CreatureBrush *>(thingBrush);
                    return UIResource::getCreatureTypeResourcePath(*brush->creatureType, Direction::South);
                }
                default:
                    break;
            }
        }
        default:
            break;
    }

    return QVariant();
}

void TileSetModel::setTileset(std::shared_ptr<Tileset> &&tileset)
{
    beginResetModel();
    _tileset = std::move(tileset);
    endResetModel();
}

void TileSetModel::clear()
{
    beginResetModel();
    _tileset.reset();
    endResetModel();
}

void TileSetModel::indexClicked(int index)
{
    auto brush = _tileset->get(index);
    selectBrush.fire(brush);
}

QHash<int, QByteArray> TileSetModel::roleNames() const
{
    return {
        {Qt::DisplayRole, "display"},
        {Roles::ImageUriRole, "imageUri"},
    };
}
