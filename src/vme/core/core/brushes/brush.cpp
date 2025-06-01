#include "brush.h"

#include <utility>

#include "../items.h"
#include "../map.h"
#include "../tile.h"
#include "border_brush.h"
#include "brushes.h"
#include "creature_brush.h"
#include "doodad_brush.h"
#include "ground_brush.h"
#include "mountain_brush.h"
#include "raw_brush.h"
#include "wall_brush.h"


std::unordered_set<Position> CircularBrushShape::Size3x3 = {
    Position(1, 0, 0),

    Position(0, 1, 0),
    Position(1, 1, 0),
    Position(2, 1, 0),

    Position(1, 2, 0),
};

Brush::Brush(std::string name)
    : _name(std::move(name)) {}

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

void Brush::updatePreview(int variation)
{
    // Empty
}

void Brush::applyWithoutBorderize(MapView &mapView, const Position &position)
{
    apply(mapView, position);
}

int Brush::variationCount() const
{
    return 1;
}

DrawItemType::DrawItemType(uint32_t serverId, Position relativePosition)
    : itemType(Items::items.getItemTypeByServerId(serverId)), relativePosition(relativePosition) {}

DrawItemType::DrawItemType(ItemType *itemType, Position relativePosition)
    : itemType(itemType), relativePosition(relativePosition) {}


//>>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>Brush Shapes>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>>

RectangularBrushShape::RectangularBrushShape(uint16_t width, uint16_t height)
    : width(width), height(height) {}

std::unordered_set<Position> RectangularBrushShape::getRelativePositions() const noexcept
{
    int minX = -width / 2;
    int maxX = width / 2;

    int minY = -height / 2;
    int maxY = height / 2;

    std::unordered_set<Position> positions;
    for (int y = minY; y < maxY; ++y)
    {
        for (int x = minX; x < maxX; ++x)
        {
            positions.emplace(Position(x, y, 0));
        }
    }

    return positions;
}

CircularBrushShape::CircularBrushShape(uint16_t radius)
    : radius(radius) {}

std::unordered_set<Position> CircularBrushShape::getRelativePositions() const noexcept
{
    // TODO implement other sizes
    return Size3x3;
}

Brush::LazyGround::LazyGround(GroundBrush *brush)
    : LazyObject(brush) {}

Brush::LazyGround::LazyGround(const std::string &groundBrushId)
    : LazyObject([groundBrushId]() {
          GroundBrush *brush = Brushes::getGroundBrush(groundBrushId);
          if (!brush)
          {
              ABORT_PROGRAM(std::format("Attempted to retrieve a GroundBrush with id '{}' from a Brush::LazyGround, but the brush did not exist.", groundBrushId));
          }

          return brush;
      }) {}
