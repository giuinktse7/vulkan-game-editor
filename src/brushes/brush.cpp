#include "brush.h"

#include "../items.h"
#include "ground_brush.h"
#include "raw_brush.h"

vme_unordered_map<uint32_t, std::unique_ptr<Brush>> Brush::rawBrushes;
vme_unordered_map<uint32_t, std::unique_ptr<Brush>> Brush::groundBrushes;

Brush::Brush(std::string name)
    : _name(name) {}

const std::string &Brush::name() const noexcept
{
    return _name;
}

void Brush::setTileset(Tileset *tileset) noexcept
{
    _tileset = tileset;
}

Tileset *Brush::tileset() const noexcept
{
    return _tileset;
}

Brush *Brush::getOrCreateRawBrush(uint32_t serverId)
{
    auto found = rawBrushes.find(serverId);
    if (found == rawBrushes.end())
    {
        rawBrushes.try_emplace(serverId, std::make_unique<RawBrush>(Items::items.getItemTypeByServerId(serverId)));
    }

    return &(*rawBrushes.at(serverId));
}

GroundBrush *Brush::addGroundBrush(GroundBrush &&brush)
{
    return addGroundBrush(std::make_unique<GroundBrush>(std::move(brush)));
}

GroundBrush *Brush::addGroundBrush(std::unique_ptr<GroundBrush> &&brush)
{
    uint32_t brushId = brush->brushId();

    auto found = groundBrushes.find(brushId);
    if (found != groundBrushes.end())
    {
        VME_LOG_ERROR(
            "Could not add ground brush '" << brush->name() << "' with id '" << brushId
                                           << "'. A brush with name '" << found->second->name()
                                           << "' already uses that brush id. Tip: Change the id to something else.");
        return nullptr;
    }

    auto result = groundBrushes.emplace(brushId, std::move(brush));

    return static_cast<GroundBrush *>(result.first.value().get());
}
