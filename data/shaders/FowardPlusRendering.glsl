// #version 330 core

// #ifndef BLOCK_SIZE
// #pragma message("BLOCK_SIZE not defined, defaulting to 16")
// #define BLOCK_SIZE 16
// #endif

// struct ComputeShaderInput {
//   uint3 groupID : SV_GroupID;
//   uint3 groupThreadID : SV_GroupThreadID;
//   uint3 dispatchThreadID : SV_DispatchThreadID;
//   uint groupIndex : SV_GroupIndex;
// };

#version 430 core
layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

uniform ivec2 uScreenSize;  // Screen dimensions in pixels

struct Plane {
    vec3 normal;
    float distance;
};

Plane ComputePlane(vec3 pointA, vec3 pointB, vec3 pointC) {
  Plane plane;
  vec3 edge1 = pointB - pointA;
  vec3 edge2 = pointC - pointA;
  plane.normal = normalize(cross(edge1, edge2));
  plane.distance = -dot(plane.normal, pointA);
  return plane;
}

struct Frustum {
    Plane planes[6]; // 0: Near, 1: Far, 2: Left, 3: Right, 4: Top, 5: Bottom
};

void main() {
  // Accessing compute shader built-in variables
  uvec3 groupID = gl_WorkGroupID;             // SV_GroupID
  uvec3 localID = gl_LocalInvocationID;       // SV_GroupThreadID
  uvec3 globalID = gl_GlobalInvocationID;     // SV_DispatchThreadID
  uint localIndex = gl_LocalInvocationIndex;  // Flattened index within the workgroup
}