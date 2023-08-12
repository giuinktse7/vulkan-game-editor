#include "ui_resources.h"

#include "core/brushes/brush.h"
#include "core/brushes/creature_brush.h"
#include "core/brushes/doodad_brush.h"
#include "core/brushes/ground_brush.h"
#include "core/brushes/raw_brush.h"
#include "core/creature.h"
#include "core/item.h"

QString UIResource::getItemPixmapString(const Item &item)
{
    return getItemPixmapString(item.serverId(), item.subtype());
}

QString UIResource::getItemPixmapString(int serverId, int subtype)
{
    return QString::fromStdString(serverId != -1 ? "image://itemTypes/" + std::to_string(serverId) + ":" + std::to_string(subtype) : "");
}

QString UIResource::getItemPixmapString(int serverId)
{
    return QString::fromStdString(serverId != -1 ? "image://itemTypes/" + std::to_string(serverId) : "");
}

QString UIResource::getCreatureTypeResourcePath(const CreatureType &creatureType, Direction direction)
{
    return QString::fromStdString(std::format("image://creatureLooktypes/{}:{}", creatureType.id(), to_underlying(direction)));
}

QString UIResource::getCreaturePixmapString(Creature *creature)
{
    return UIResource::getCreatureTypeResourcePath(creature->creatureType);
}

QString UIResource::getItemTypeResourcePath(uint32_t serverId, uint8_t subtype)
{
    if (subtype == 0)
    {
        return QString::fromStdString(std::format("image://itemTypes/{}", serverId));
    }
    else
    {
        return QString::fromStdString(std::format("image://itemTypes/{}:{}", serverId, static_cast<int>(subtype)));
    }
}

QString UIResource::resourcePath(Brush *brush)
{
    switch (brush->type())
    {
        case BrushType::Raw:
        {
            auto rawBrush = static_cast<RawBrush *>(brush);
            return getItemTypeResourcePath(rawBrush->serverId());
        }
        case BrushType::Ground:
        {
            auto groundBrush = static_cast<GroundBrush *>(brush);
            return getItemTypeResourcePath(groundBrush->iconServerId());
        }
        case BrushType::Doodad:
        {
            auto doodadBrush = static_cast<DoodadBrush *>(brush);
            return getItemTypeResourcePath(doodadBrush->iconServerId());
        }
        case BrushType::Creature:
        {
            auto creatureBrush = static_cast<CreatureBrush *>(brush);
            return getCreatureTypeResourcePath(*creatureBrush->creatureType);
        }
        default:
            break;
    }

    ABORT_PROGRAM("Could not determine resource type.");
}