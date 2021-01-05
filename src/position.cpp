#include "position.h"

#include "map_view.h"

Position::Position()
    : x(0), y(0), z(0) {}

Position::Position(Position::value_type x, Position::value_type y, int z)
    : x(x), y(y), z(z) {}

void Position::move(Position::value_type x, Position::value_type y, int z)
{
    this->x += x;
    this->y += y;
    this->z += z;
}

ScreenPosition::ScreenPosition(int x, int y)
    : BasePosition(x, y) {}

ScreenPosition::ScreenPosition()
    : BasePosition(0, 0) {}

WorldPosition::WorldPosition(WorldPosition::value_type x, WorldPosition::value_type y)
    : BasePosition(x, y) {}

WorldPosition::WorldPosition()
    : BasePosition(0, 0) {}

WorldPosition ScreenPosition::worldPos(const MapView &mapView) const
{
    WorldPosition::value_type newX = std::lroundf(mapView.x() + this->x / mapView.getZoomFactor());
    WorldPosition::value_type newY = std::lroundf(mapView.y() + this->y / mapView.getZoomFactor());

    return WorldPosition(newX, newY);
}

Position ScreenPosition::toPos(const MapView &mapView) const
{
    return worldPos(mapView).toPos(mapView.floor());
}

Position WorldPosition::toPos(const MapView &mapView) const
{
    return toPos(mapView.floor());
}

Position WorldPosition::toPos(int floor) const
{
    // Because of Tibia's perspective, movement in Z also means movement in X and Y
    int offset = floor - GROUND_FLOOR;

    auto newX = static_cast<WorldPosition::value_type>(std::floor(this->x / MapTileSize)) - offset;
    auto newY = static_cast<WorldPosition::value_type>(std::floor(this->y / MapTileSize)) - offset;

    return Position(newX, newY, floor);
}

WorldPosition Position::worldPos() const noexcept
{
    // Because of Tibia's perspective, movement in Z also means movement in X and Y
    int offset = z - GROUND_FLOOR;
    return WorldPosition((x + offset) * MapTileSize, (y + offset) * MapTileSize);
}

/**
 * Returns a list of positions on a line starting at 'from' and ending at 'to'
 * The positions will never include 'from', but always include 'to'.
 */
std::vector<Position> Position::bresenHams(Position from, Position to)
{
    DEBUG_ASSERT(from.z == to.z, "from and to must be on the same floor.");
    std::vector<Position> result;
    if (from == to)
        return result;

    int x0 = from.x;
    int y0 = from.y;
    int x1 = to.x;
    int y1 = to.y;

    int dx = std::abs(x1 - x0);
    int sx = x0 < x1 ? 1 : -1;

    int dy = -std::abs(y1 - y0);
    int sy = y0 < y1 ? 1 : -1;

    int err = dx + dy;
    while (true)
    {
        if (!(x0 == from.x && y0 == from.y))
            result.emplace_back(x0, y0, from.z);
        if (x0 == x1 && y0 == y1)
            return result;

        int e2 = 2 * err;
        if (e2 >= dy)
        {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx)
        {
            err += dx;
            y0 += sy;
        }
    }
}

uint32_t Position::tilesInRegion(const Position &from, const Position &to)
{
    uint32_t dx = static_cast<uint32_t>(std::max(1, std::abs(to.x - from.x)));
    uint32_t dy = static_cast<uint32_t>(std::max(1, std::abs(to.y - from.y)));
    uint32_t dz = static_cast<uint32_t>(std::max(1, std::abs(to.z - from.z)));

    return dx * dy * dz;
}
