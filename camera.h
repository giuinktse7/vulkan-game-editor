#pragma once

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
struct ScreenPosition;

class Camera
{

public:
  glm::vec2 position = glm::vec2(0 * 32, 0 * 32);
  int floor = 7;
  float zoomFactor = 1.0f;

  struct
  {
    bool left = false;
    bool right = false;
    bool up = false;
    bool down = false;
  } keys;

  void setPosition(glm::vec2 position);

  void translate(glm::vec2 delta);
  void translateZ(int z);

  void updateZoom(ScreenPosition cursorPos);

  void zoomIn();

  void zoomOut();

  void resetZoom();

private:
  // Possible zoom steps, [0, 20]
  int zoomSteps = 20;

  // Current zoom step
  int zoomStep = 10;

  bool zoomChanged = false;

  void setZoomStep(int zoomStep);
};