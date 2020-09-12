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

  void setPosition(WorldPosition position);
  inline void setX(long x);
  inline void setY(long y);

  void translate(WorldPosition delta);
  void translateZ(int z);

  void updateZoom(ScreenPosition cursorPos);

  void zoomIn();

  void zoomOut();

  void resetZoom();

  inline constexpr long x() const
  {
    return _position.x;
  }

  inline constexpr long y() const
  {
    return _position.y;
  }

  inline float zoomFactor() const;
  inline WorldPosition position() const;

private:
  int zoomStep;
  bool zoomChanged;

  float _zoomFactor;
  WorldPosition _position;

  void setZoomStep(int zoomStep);
};

inline void Camera::setX(long x)
{
  _position.x = x;
}
inline void Camera::setY(long y)
{
  _position.y = y;
}

inline float Camera::zoomFactor() const
{
  return _zoomFactor;
}

inline WorldPosition Camera::position() const
{
  return _position;
}
