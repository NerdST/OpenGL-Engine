#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in uint aWallData;

#define PI 3.14159265359
#define gridSize 1.0

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;

uniform mat4 view;
uniform mat4 projection;

void main() {
  uint rotation = (aWallData >> 14) & 0x03u;  // Extract 2 rotation bits
  uint xPos = (aWallData >> 7) & 0x7Fu;       // Extract 7 x bits
  uint zPos = aWallData & 0x7Fu;              // Extract 7 z bits

  vec3 worldPos = vec3(float(xPos) * gridSize, 0.0, float(zPos) * gridSize);

  // Apply rotation based on wall data
  float angle = float(rotation) * (PI / 2.0);

  mat4 rotationMatrix = mat4(
    cos(angle), 0.0, sin(angle), 0.0,
    0.0,        1.0, 0.0,        0.0,
    -sin(angle), 0.0, cos(angle), 0.0,
    0.0,        0.0, 0.0,        1.0
  );

  mat4 translationMatrix = mat4(
    1.0, 0.0, 0.0, 0.0,
    0.0, 1.0, 0.0, 0.0,
    0.0, 0.0, 1.0, 0.0,
    worldPos.x, worldPos.y, worldPos.z, 1.0
  );

  mat4 instanceModel = translationMatrix * rotationMatrix;
  
  vec4 worldPosition = instanceModel * vec4(aPos, 1.0);
  FragPos = worldPosition.xyz;
  Normal = mat3(transpose(inverse(instanceModel))) * aNormal;
  TexCoords = aTexCoords;

  gl_Position = projection * view * worldPosition;
}
