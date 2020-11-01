#pragma once

#define GLM_FORCE_RADIANS
#include <functional>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "const.h"
#include "position.h"
#include "util.h"
#include <algorithm>

class MapRegion;

class Camera
{
public:
  struct Viewport
  {
    WorldPosition::value_type x;
    WorldPosition::value_type y;
    int z;
    WorldPosition::value_type width;
    WorldPosition::value_type height;
    float zoom;
  };

  std::function<void()> onViewportChanged = []() {};

  Camera();

  void setWorldPosition(WorldPosition position) noexcept;
  inline void setX(WorldPosition::value_type x) noexcept;
  inline void setY(WorldPosition::value_type y) noexcept;
  inline void setZ(int z) noexcept;
  inline void setSize(WorldPosition::value_type width, WorldPosition::value_type height) noexcept;

  void translate(WorldPosition delta);
  void translateZ(int z);

  void zoomIn(ScreenPosition zoomOrigin);
  void zoomOut(ScreenPosition zoomOrigin);
  void resetZoom(ScreenPosition zoomOrigin);

  inline constexpr WorldPosition::value_type x() const noexcept;
  inline constexpr WorldPosition::value_type y() const noexcept;
  inline constexpr int z() const noexcept;

  inline float zoomFactor() const noexcept;
  inline Position position() const noexcept;
  inline WorldPosition worldPosition() const noexcept;

  inline const Viewport &viewport() const noexcept;

private:
  Viewport _viewport;

  int _zoomStep;

  void updateZoom(ScreenPosition cursorPos);
  void setZoomStep(int zoomStep, ScreenPosition zoomOrigin);

  float computeZoomFactor() const;
};

inline constexpr WorldPosition::value_type Camera::x() const noexcept
{
  return _viewport.x;
}

inline constexpr WorldPosition::value_type Camera::y() const noexcept
{
  return _viewport.y;
}

inline constexpr int Camera::z() const noexcept
{
  return _viewport.z;
}

inline void Camera::setX(WorldPosition::value_type x) noexcept
{
  WorldPosition::value_type oldX = _viewport.x;

  _viewport.x = std::max(x, 0);

  if (oldX != _viewport.x)
    onViewportChanged();
}

inline void Camera::setY(WorldPosition::value_type y) noexcept
{
  WorldPosition::value_type oldY = _viewport.y;

  _viewport.y = std::max(y, 0);

  if (oldY != _viewport.y)
    onViewportChanged();
}

inline void Camera::setZ(int z) noexcept
{
  WorldPosition::value_type oldZ = _viewport.z;

  _viewport.z = std::clamp(z, 0, MAP_LAYERS - 1);

  if (oldZ != _viewport.z)
    onViewportChanged();
}

inline void Camera::setSize(WorldPosition::value_type width, WorldPosition::value_type height) noexcept
{
  bool changed = _viewport.width != width || _viewport.height != height;
  _viewport.width = width;
  _viewport.height = height;

  if (changed)
    onViewportChanged();
}

inline float Camera::zoomFactor() const noexcept
{
  return _viewport.zoom;
}

inline Position Camera::position() const noexcept
{
  return Position(_viewport.x / MapTileSize, _viewport.y / MapTileSize, _viewport.z);
}

inline WorldPosition Camera::worldPosition() const noexcept
{
  return WorldPosition(_viewport.x, _viewport.y);
}

inline const Camera::Viewport &Camera::viewport() const noexcept
{
  return _viewport;
}
