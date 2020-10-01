#include "position.h"

#include "map_view.h"

Position::Position() : BasePosition(0, 0), z(0) {}
Position::Position(long x, long y, int z) : BasePosition(x, y), z(z) {}

void Position::move(long x, long y, int z)
{
  this->x += x;
  this->y += y;
  this->z += z;
}

ScreenPosition::ScreenPosition(int x, int y) : BasePosition(x, y) {}
ScreenPosition::ScreenPosition() : BasePosition(0, 0) {}

WorldPosition::WorldPosition(long x, long y) : BasePosition(x, y) {}
WorldPosition::WorldPosition() : BasePosition(0, 0) {}

MapPosition::MapPosition(long x, long y) : BasePosition(x, y) {}
MapPosition::MapPosition() : BasePosition(0, 0) {}

WorldPosition ScreenPosition::worldPos(const MapView &mapView) const
{
  long newX = std::lroundf(mapView.x() + this->x / mapView.getZoomFactor());
  long newY = std::lroundf(mapView.y() + this->y / mapView.getZoomFactor());

  return WorldPosition(newX, newY);
}

MapPosition ScreenPosition::mapPos(const MapView &mapView) const
{
  long newX = std::lroundf((this->x / mapView.getZoomFactor()) / MapTileSize);
  long newY = std::lroundf((this->y / mapView.getZoomFactor()) / MapTileSize);

  return MapPosition(newX, newY);
}

MapPosition WorldPosition::mapPos() const
{
  return MapPosition{
      static_cast<long>(std::floor(this->x / MapTileSize)),
      static_cast<long>(std::floor(this->y / MapTileSize))};
}

WorldPosition MapPosition::worldPos() const
{
  return WorldPosition(this->x * MapTileSize, this->y * MapTileSize);
}

Position MapPosition::floor(int floor) const
{
  return Position(this->x, this->y, floor);
}

Position ScreenPosition::toPos(const MapView &mapView) const
{
  return worldPos(mapView).mapPos().floor(mapView.getFloor());
}

Position WorldPosition::toPos(const MapView &mapView) const
{
  return toPos(mapView.getFloor());
}

Position WorldPosition::toPos(int floor) const
{
  return mapPos().floor(floor);
}
