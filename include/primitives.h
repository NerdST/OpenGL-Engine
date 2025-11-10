#ifndef PRIMITIVES_H
#define PRIMITIVES_H

#include "mesh.h"

class Plane : public Mesh
{
public:
  Plane(float width, float height, std::vector<Texture> textures)
      : Mesh(generateVertices(width, height), generateIndices(), textures)
  {
  }

private:
  // clang-format off
  std::vector<Vertex> generateVertices(float width, float height)
  {
    return {
      {{-width / 2, 0.0f, -height / 2}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
      {{ width / 2, 0.0f, -height / 2}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
      {{ width / 2, 0.0f,  height / 2}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
      {{-width / 2, 0.0f,  height / 2}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
    };
  }

  std::vector<unsigned int> generateIndices()
  {
    return {
      0, 1, 2,
      2, 3, 0,
    };
  }
  // clang-format on
};

class RectangularPrism : public Mesh
{
public:
  RectangularPrism(float length, float width, float height, std::vector<Texture> textures)
      : Mesh(generateVertices(length, width, height), generateIndices(), textures)
  {
  }

private:
  // clang-format off
  std::vector<Vertex> generateVertices(float length, float width, float height)
  {
    return {
      // Bottom face
      {{-length / 2, 0.0f, -width / 2}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},
      {{ length / 2, 0.0f, -width / 2}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},
      {{ length / 2, 0.0f,  width / 2}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}},
      {{-length / 2, 0.0f,  width / 2}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},

      // Top face
      {{-length / 2, height, -width / 2}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
      {{ length / 2, height, -width / 2}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
      {{ length / 2, height,  width / 2}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
      {{-length / 2, height,  width / 2}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
    };
  }

  std::vector<unsigned int> generateIndices()
  {
    return {
      // Bottom face
      0, 1, 2,
      2, 3, 0,

      // Top face
      4, 5, 6,
      6, 7, 4,

      // Front face
      3, 2, 6,
      6, 7, 3,

      // Back face
      0, 1, 5,
      5, 4, 0,

      // Left face
      0, 3, 7,
      7, 4, 0,

      // Right face
      1, 2, 6,
      6, 5, 1,
    };
  }
  // clang-format on
};

class Sphere : public Mesh
{
public:
  Sphere(float radius, unsigned int sectorCount, unsigned int stackCount, std::vector<Texture> textures)
      : Mesh(generateVertices(radius, sectorCount, stackCount), generateIndices(sectorCount, stackCount), textures)
  {
  }

private:
  std::vector<Vertex> generateVertices(float radius, unsigned int sectorCount, unsigned int stackCount)
  {
    std::vector<Vertex> vertices;
    for (unsigned int i = 0; i <= stackCount; ++i)
    {
      float stackAngle = glm::pi<float>() / 2 - i * glm::pi<float>() / stackCount; // from pi/2 to -pi/2
      float xy = radius * cosf(stackAngle);                                        // r * cos(u)
      float z = radius * sinf(stackAngle);                                         // r * sin(u)
      for (unsigned int j = 0; j <= sectorCount; ++j)
      {
        float sectorAngle = j * 2 * glm::pi<float>() / sectorCount; // from 0 to 2pi

        float x = xy * cosf(sectorAngle); // r * cos(u) * cos(v)
        float y = xy * sinf(sectorAngle); // r * cos(u) * sin(v)

        glm::vec3 position = glm::vec3(x, y, z);
        glm::vec3 normal = glm::normalize(position);
        glm::vec2 texCoords = glm::vec2((float)j / sectorCount, (float)i / stackCount);

        vertices.push_back({position, normal, texCoords});
      }
    }
    return vertices;
  }
  std::vector<unsigned int> generateIndices(unsigned int sectorCount, unsigned int stackCount)
  {
    std::vector<unsigned int> indices;
    for (unsigned int i = 0; i < stackCount; ++i)
    {
      unsigned int k1 = i * (sectorCount + 1); // beginning of current stack
      unsigned int k2 = k1 + sectorCount + 1;  // beginning of next stack

      for (unsigned int j = 0; j < sectorCount; ++j, ++k1, ++k2)
      {
        if (i != 0)
        {
          indices.push_back(k1);
          indices.push_back(k2);
          indices.push_back(k1 + 1);
        }
        if (i != (stackCount - 1))
        {
          indices.push_back(k1 + 1);
          indices.push_back(k2);
          indices.push_back(k2 + 1);
        }
      }
    }
    return indices;
  }
};

/** MAZE IMPLEMENTATION FOR REFERENCE
class Wall : public Mesh
{
public:
  Wall(float width, float height, Shader shader)
      : Mesh(generateVertices(width, height), generateIndices(), std::vector<Texture>(), shader)
  {
    // Additional setup for the wall if needed
  }

private:
  // clang-format off
  std::vector<Vertex> generateVertices(float width, float height)
  {
    // return {
    //     -width / 2, 0.0f, -height / 2, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
    //     width / 2, 0.0f, -height / 2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
    //     width / 2, 0.0f, height / 2, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
    //     -width / 2, 0.0f, height / 2, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
    // };
    return {
      {{-width / 2, 0.0f, -height / 2}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
      {{ width / 2, 0.0f, -height / 2}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
      {{ width / 2, 0.0f,  height / 2}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
      {{-width / 2, 0.0f,  height / 2}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
    };
  }

  std::vector<unsigned int> generateIndices()
  {
    return {
      0, 1, 2,
      2, 3, 0,
    };
  }
  // clang-format on
};

class MazeWalls : public Mesh
{
public:
  MazeWalls(float width, float height, std::vector<Texture> wallTextures, Shader shader)
      : Mesh(generateVertices(width, height), generateIndices(), wallTextures, shader)
  {
    generateMazeWalls();
    generateWallData();
    setupInstancedMesh();
  }

  void DrawInstanced(Shader &shader)
  {
    if (wallData.empty())
      return;

    // Update instance buffer if needed
    updateInstanceBuffer();

    // Use the parent's instanced drawing method
    Mesh::DrawInstanced(shader, wallData.size());
  }

  void addWall(uint8_t x, uint8_t z, uint8_t rotation = 0)
  {
    WallSegment segment;
    segment.rotation = rotation & 0x03; // Ensure only 2 bits
    segment.x = x & 0x7F;               // Ensure only 7 bits
    segment.z = z & 0x7F;               // Ensure only 7 bits

    uint16_t packedData = (segment.rotation & 0x03) << 14 |
                          (segment.x & 0x7F) << 7 |
                          (segment.z & 0x7F);
    wallData.push_back(packedData);
    instanceBufferNeedsUpdate = true;
  }

  void clearWalls()
  {
    wallData.clear();
    instanceBufferNeedsUpdate = true;
  }

  size_t getWallCount() const { return wallData.size(); }

private:
  // The maze buffer should have position and rotation data for each wall segment
  // 2 bits for rotation (0, 90, 180, 270)
  // 14 bits for position, 7 bits each for x and z (0-127)
  // Total 16 bits = 2 bytes per wall segment
  // This 128x128 area will be per chunk, and multiple chunks can be rendered
  // together by combining their wall data buffers
  std::vector<uint16_t> wallData;
  unsigned int instanceVBO;
  bool instanceBufferNeedsUpdate;

  struct WallSegment
  {
    uint8_t rotation; // 0, 1, 2, 3 for 0, 90, 180, 270 degrees
    uint8_t x;        // 0-127
    uint8_t z;        // 0-127
  };

  void generateWallData()
  {
    // Generate a simple test pattern - you can replace this with maze generation
    wallData.clear();

    // Create a simple room outline
    for (int x = 0; x < 10; x++)
    {
      addWall(x, 0, 0); // Bottom wall
      addWall(x, 9, 2); // Top wall
    }
    for (int z = 1; z < 9; z++)
    {
      addWall(0, z, 3); // Left wall
      addWall(9, z, 1); // Right wall
    }
  }

  // clang-format off
  std::vector<Vertex> generateVertices(float width, float height)
  {
    // return {
    //     -width / 2, 0.0f, -height / 2, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
    //     width / 2, 0.0f, -height / 2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
    //     width / 2, 0.0f, height / 2, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
    //     -width / 2, 0.0f, height / 2, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
    // };
    return {
      {{-width / 2, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}}, // Bottom left
      {{ width / 2, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}}, // Bottom right
      {{ width / 2, height, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}}, // Top right
      {{-width / 2, height, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}}, // Top left
    };
  }

  std::vector<unsigned int> generateIndices()
  {
    return {
      0, 1, 2,
      2, 3, 0,
    };
  }

  void generateMazeWalls()
  {
    // Implementation for generating maze walls
    // super basic square room for now
  }
  // clang-format on

  void setupInstancedMesh()
  {
    // Call parent setup first
    setupMesh();

    // Generate instance buffer
    glGenBuffers(1, &instanceVBO);
    instanceBufferNeedsUpdate = true;

    // Bind VAO and setup instance attributes
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);

    // Set up instance attribute (wall data)
    glEnableVertexAttribArray(3);
    glVertexAttribIPointer(3, 1, GL_UNSIGNED_SHORT, sizeof(uint16_t), (void *)0);
    glVertexAttribDivisor(3, 1); // One per instance

    glBindVertexArray(0);
  }

  void updateInstanceBuffer()
  {
    if (!instanceBufferNeedsUpdate || wallData.empty())
      return;

    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, wallData.size() * sizeof(uint16_t),
                 wallData.data(), GL_DYNAMIC_DRAW);

    instanceBufferNeedsUpdate = false;
  }
};
*/

#endif // PRIMITIVES_H