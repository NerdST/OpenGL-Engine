// Debug fragment shader to visualize depth
// gPosition.w contains linear depth, this program
// normalizes it for visualization

#version 330 core
layout(location = 1) out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D gPosition;
uniform float nearPlane;
uniform float farPlane;

void main()
{
    float depth = texture(gPosition, TexCoords).w;
    
    // Normalize depth to 0-1 range for visualization
    // You can adjust farPlane to control the visualization range
    float normalizedDepth = depth / farPlane;
    normalizedDepth = clamp(normalizedDepth, 0.0, 1.0);
    
    // Display as grayscale (black = near, white = far)
    FragColor = vec4(vec3(normalizedDepth), 1.0);
}
