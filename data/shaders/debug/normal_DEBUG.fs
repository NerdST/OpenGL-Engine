// Debug fragment shader to visualize normals
// gNormal buffer has values from [-1,1], this program
// expands that to [0-255] for visualization

#version 330 core
layout(location = 0) out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D gNormal;

void main()
{
    vec3 normal = texture(gNormal, TexCoords).rgb;
    FragColor = vec4(normal * 0.5 + 0.5, 1.0);
}