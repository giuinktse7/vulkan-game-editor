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

TileSetModel::TileSetModel() {}

int TileSetModel::rowCount(const QModelIndex &) const
{
    if (!_tileset)
    {
        return 0;
    }

    return static_cast<int>(std::ceil(_tileset->size() / columnCount()));
}

int TileSetModel::columnCount(const QModelIndex &) const
{
    return 1;
}

QVariant TileSetModel::data(const QModelIndex &modelIndex, int role) const
{
    if (!_tileset)
    {
        return QVariant();
    }

    switch (role)
    {
        case Qt::DisplayRole:
            return QString("%1, %2").arg(modelIndex.column()).arg(modelIndex.row());
        case Roles::ImageUriRole:
        {
            uint32_t index = modelIndex.row();
            Brush *thingBrush = _tileset->get(index);

            switch (thingBrush->type())
            {
                case BrushType::Raw:
                {
                    auto brush = static_cast<RawBrush *>(thingBrush);
                    uint32_t serverId = brush->serverId();
                    return UIResource::getItemPixmapString(serverId);
                }
                case BrushType::Ground:
                {
                    auto brush = static_cast<GroundBrush *>(thingBrush);
                    uint32_t serverId = brush->iconServerId();
                    return UIResource::getItemPixmapString(serverId);
                }
                case BrushType::Border:
                {
                    auto brush = static_cast<BorderBrush *>(thingBrush);
                    uint32_t serverId = brush->iconServerId();
                    return UIResource::getItemPixmapString(serverId);
                    break;
                }
                case BrushType::Wall:
                {
                    auto brush = static_cast<WallBrush *>(thingBrush);
                    uint32_t serverId = brush->iconServerId();
                    return UIResource::getItemPixmapString(serverId);
                    break;
                }
                case BrushType::Mountain:
                {
                    auto brush = static_cast<MountainBrush *>(thingBrush);
                    uint32_t serverId = brush->iconServerId();
                    return UIResource::getItemPixmapString(serverId);
                    break;
                }
                case BrushType::Doodad:
                {
                    auto brush = static_cast<DoodadBrush *>(thingBrush);
                    uint32_t serverId = brush->iconServerId();
                    return UIResource::getItemPixmapString(serverId);
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

int TileSetModel::indexOfBrush(Brush *brush)
{
    if (!_tileset)
    {
        return -1;
    }

    return _tileset->indexOf(brush);
}

void TileSetModel::setTileset(Tileset *tileset)
{
    beginResetModel();
    _tileset = tileset;
    endResetModel();
}

Brush *TileSetModel::getBrush(int index)
{
    if (!_tileset)
    {
        return nullptr;
    }

    return _tileset->get(index);
}

void TileSetModel::clear()
{
    beginResetModel();
    _tileset = nullptr;
    endResetModel();
}

void TileSetModel::indexClicked(int index)
{
    if (!_tileset)
    {
        return;
    }

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

#include "moc_tileset_model.cpp"