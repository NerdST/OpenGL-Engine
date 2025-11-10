#version 330 core

layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec2 aTex;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 Tex;
} vs_out;

void main() {
    vec4 world = model * vec4(aPos,1.0);
    vs_out.FragPos = world.xyz;
    vs_out.Normal = mat3(transpose(inverse(model))) * aNormal;
    vs_out.Tex = aTex;
    gl_Position = projection * view * world;
}