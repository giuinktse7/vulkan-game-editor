#include "ui_resources.h"

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