#version 330 core
out vec4 FragColor;

uniform vec3 lightColor;
uniform float alpha;

void main()
{
    FragColor = vec4(lightColor, alpha);
}