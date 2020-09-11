#include "position.h"

#include "map_view.h"

Position::Position() : BasePosition(0, 0), z(0) {}
Position::Position(long x, long y, int z) : BasePosition(x, y), z(z) {}

ScreenPosition::ScreenPosition(double x, double y) : BasePosition(x, y) {}
ScreenPosition::ScreenPosition() : BasePosition(0, 0) {}

WorldPosition::WorldPosition(double x, double y) : BasePosition(x, y) {}
WorldPosition::WorldPosition() : BasePosition(0, 0) {}

MapPosition::MapPosition(long x, long y) : BasePosition(x, y) {}
MapPosition::MapPosition() : BasePosition(0, 0) {}

WorldPosition ScreenPosition::worldPos(const MapView &mapView) const
{
  double newX = mapView.getX() + this->x / mapView.getZoomFactor();
  double newY = mapView.getY() + this->y / mapView.getZoomFactor();

  return WorldPosition(newX, newY);
}

MapPosition ScreenPosition::mapPos(const MapView &mapView) const
{
  double newX = this->x / mapView.getZoomFactor();
  double newY = this->y / mapView.getZoomFactor();

  newX = std::floor(newX / MapTileSize);
  newY = std::floor(newY / MapTileSize);

  return MapPosition{static_cast<long>(newX), static_cast<long>(newY)};
}

MapPosition WorldPosition::mapPos() const
{
  return MapPosition{
      static_cast<long>(std::floor(this->x / MapTileSize)),
      static_cast<long>(std::floor(this->y / MapTileSize))};
}

WorldPosition MapPosition::worldPos() const
{
  return WorldPosition{static_cast<double>(this->x * MapTileSize), static_cast<double>(this->y * MapTileSize)};
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
