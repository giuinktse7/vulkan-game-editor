#pragma once

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include "const.h"
#include "position.h"
#include "util.h"

class Camera
{

public:
  Camera();
  int floor;

  struct
  {
    bool left = false;
    bool right = false;
    bool up = false;
    bool down = false;
  } keys;

  void setPosition(WorldPosition position) noexcept;
  inline void setX(WorldPosition::value_type x) noexcept;
  inline void setY(WorldPosition::value_type y) noexcept;

  void translate(WorldPosition delta);
  void translateZ(int z);

  void zoomIn(ScreenPosition zoomOrigin);
  void zoomOut(ScreenPosition zoomOrigin);
  void resetZoom(ScreenPosition zoomOrigin);

  inline constexpr WorldPosition::value_type x() const noexcept
  {
    return _position.x;
  }

  inline constexpr WorldPosition::value_type y() const noexcept
  {
    return _position.y;
  }

  inline float zoomFactor() const noexcept;
  inline WorldPosition position() const noexcept;

private:
  int zoomStep;

  float _zoomFactor;
  WorldPosition _position;

  void updateZoom(ScreenPosition cursorPos);
  void setZoomStep(int zoomStep, ScreenPosition zoomOrigin);
};

inline void Camera::setX(WorldPosition::value_type x) noexcept
{
  _position.x = std::max(x, 0);
}
inline void Camera::setY(WorldPosition::value_type y) noexcept
{
  _position.y = std::max(y, 0);
}

inline float Camera::zoomFactor() const noexcept
{
  return _zoomFactor;
}

inline WorldPosition Camera::position() const noexcept
{
  return _position;
}
