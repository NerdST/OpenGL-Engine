#ifndef LIGHTS_H
#define LIGHTS_H

#include <glm/glm.hpp>
#include "primitives.h"

struct PointLight
{
  glm::vec3 pos{0.0f};
  glm::vec3 color{1.0f};
  float intensity{1.0f};
  float radius{1.0f};
  Sphere lightVolume{0.02f, 36, 18, {}}; // default sphere
};

#endif // LIGHTS_H