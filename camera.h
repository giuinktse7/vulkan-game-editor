#pragma once

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>

class Camera
{

public:
  glm::vec3 position = glm::vec3(0 * 32, 0 * 32, 7);

  float zoomFactor = 1.0f;

  struct
  {
    bool left = false;
    bool right = false;
    bool up = false;
    bool down = false;
  } keys;

  void setPosition(glm::vec3 position);

  void translate(glm::vec3 delta);
  void translateZ(int z);

  void updateZoom();

  void zoomIn();

  void zoomOut();

  void resetZoom();

private:
  // Possible zoom steps, [0, 20]
  int zoomSteps = 20;

  // Current zoom step
  int zoomStep = 10;

  void setZoomStep(int zoomStep);
};