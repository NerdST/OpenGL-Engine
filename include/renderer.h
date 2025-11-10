#ifndef RENDERER_H
#define RENDERER_H

#include <vector>
#include "mesh.h"

class Renderer
{
public:
  virtual void render() = 0;
  void setup()
  {
    // Setup code for the renderer
  }

  void render()
  {
  }

private:
  // Instanced primitives
};

#endif // RENDERER_H