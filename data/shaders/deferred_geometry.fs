#version 330 core
layout (location = 0) out vec4 gPosition;// xyz position, w = linear depth (32-bit float)
layout (location = 1) out vec4 gNormal;// xyz normal, w = unused/packed (16-bit float or 8-bit unsigned norm)
layout (location = 2) out vec4 gAlbedoSpec;// rgb albedo, a = specular/roughness (8-bit unsigned norm)
layout (location = 3) out vec4 gMatProps;// r = metallic, g = roughness, b = ambient occlusion (AO), a = height
layout (location = 4) out vec4 gEmissive;// rgb emissive color, a = unused

#define MIN_LAYERS 8.0
#define MAX_LAYERS 32.0
#define HEIGHT_SCALE 0.1
#define INVERT_HEIGHT true

in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
    mat3 TBN;
} vs_out;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;
uniform sampler2D texture_normal1;
uniform sampler2D texture_metallic1;
uniform sampler2D texture_height1;
uniform sampler2D texture_roughness1;
uniform sampler2D texture_ao1;
uniform sampler2D texture_emissive1;

uniform vec3 viewPos;
uniform bool useParallaxMapping;

vec2 ParallaxMapping(vec2 texCoords, vec3 viewDir, sampler2D heightMap) {
    // Number of layers based on view angle (more layers when viewing at steep angles)
    float numLayers = mix(MAX_LAYERS, MIN_LAYERS, abs(dot(vec3(0.0, 0.0, 1.0), viewDir)));
    float layerDepth = 1.0 / numLayers;
    float currentLayerDepth = 0.0;
    vec2 P = viewDir.xy / viewDir.z * HEIGHT_SCALE;
    vec2 deltaTexCoords = P / numLayers;

    // Initial values
    vec2 currentTexCoords = texCoords;
    float currentDepthMapValue = INVERT_HEIGHT ? (1.0 - texture(heightMap, currentTexCoords).r) : texture(heightMap, currentTexCoords).r;

    // Steep parallax mapping loop
    while (currentLayerDepth < currentDepthMapValue) {
        currentTexCoords -= deltaTexCoords;
        currentDepthMapValue = INVERT_HEIGHT ? (1.0 - texture(heightMap, currentTexCoords).r) : texture(heightMap, currentTexCoords).r;
        currentLayerDepth += layerDepth;
    }

    // === Parallax Occlusion Mapping (smooth interpolation) ===
    // Get texture coordinates before collision
    vec2 prevTexCoords = currentTexCoords + deltaTexCoords;

    // Get depth after and before collision for linear interpolation
    float afterDepth = currentDepthMapValue - currentLayerDepth;
    float beforeDepth = (INVERT_HEIGHT ? (1.0 - texture(heightMap, prevTexCoords).r) : texture(heightMap, prevTexCoords).r) - currentLayerDepth + layerDepth;

    // Interpolation weight
    float weight = afterDepth / (afterDepth - beforeDepth);
    vec2 finalTexCoords = prevTexCoords * weight + currentTexCoords * (1.0 - weight);

    return finalTexCoords;
}

float linearDepth(vec3 fragPos) {
    // assuming camera at view transform (pass near/far if needed)
    return length(fragPos);
}

// Octahedral encoding for normals (saves 1 channel)
vec2 encodeNormal(vec3 n) {
    n /= (abs(n.x) + abs(n.y) + abs(n.z));
    return n.z >= 0. ? n.xy : (1. - abs(n.yx)) * sign(n.xy);
}

void main() {
    // Calculate view direction in tangent space
    vec3 viewDirWorld = normalize(viewPos - vs_out.FragPos);
    vec3 viewDir = normalize(transpose(vs_out.TBN) * viewDirWorld);

    // Apply parallax mapping only if enabled and viewDir.z is positive
    vec2 texCoords = vs_out.TexCoords;
    texCoords = ParallaxMapping(vs_out.TexCoords, viewDir, texture_height1);

    vec3 normalMap = texture(texture_normal1, texCoords).rgb;
    normalMap = normalize(normalMap * 2.0 - 1.0);

    vec3 normal = normalize(vs_out.TBN * normalMap);

    vec3 albedo = texture(texture_diffuse1, texCoords).rgb;
    float specular = texture(texture_specular1, texCoords).r;
    float metallic = texture(texture_metallic1, texCoords).r;
    float height = texture(texture_height1, texCoords).r;
    float roughness = texture(texture_roughness1, texCoords).r;
    float ao = texture(texture_ao1, texCoords).r;
    vec3 emissive = texture(texture_emissive1, texCoords).rgb;

    // vec3 normal = normalize(fs_in.Normal);

    gPosition = vec4(vs_out.FragPos, linearDepth(vs_out.FragPos));
    gNormal = vec4(normal, 1.);
    gAlbedoSpec = vec4(albedo, specular);
    gMatProps = vec4(metallic, roughness, ao, height);
    gEmissive = vec4(emissive, 1.);
}