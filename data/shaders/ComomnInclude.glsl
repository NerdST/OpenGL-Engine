#version 330 core

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