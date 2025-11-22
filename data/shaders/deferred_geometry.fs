#version 330 core
layout(location=0) out vec4 gPosition;     // xyz position, w = linear depth (32-bit float)
layout(location=1) out vec4 gNormal;       // xyz normal, w = unused/packed (16-bit float or 8-bit unsigned norm)
layout(location=2) out vec4 gAlbedoSpec;   // rgb albedo, a = specular/roughness (8-bit unsigned norm)
layout(location=3) out vec4 gMatProps;     // r = metallic, g = roughness, b = ambient occlusion (AO)
layout(location=4) out vec4 gEmissive;    // rgb emissive color, a = unused

in VS_OUT { vec3 FragPos; vec3 Normal; vec2 Tex; } fs_in;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;
uniform sampler2D texture_metallic1;
uniform sampler2D texture_roughness1;
uniform sampler2D texture_ao1;
uniform sampler2D texture_emissive1;

float linearDepth(vec3 fragPos) {
    // assuming camera at view transform (pass near/far if needed)
    return length(fragPos);
}

// Octahedral encoding for normals (saves 1 channel)
vec2 encodeNormal(vec3 n) {
    n /= (abs(n.x) + abs(n.y) + abs(n.z));
    return n.z >= 0.0 ? n.xy : (1.0 - abs(n.yx)) * sign(n.xy);
}

void main() {
    vec3 albedo = texture(texture_diffuse1, fs_in.Tex).rgb;
    float specular = texture(texture_specular1, fs_in.Tex).r;
    float metallic = texture(texture_metallic1, fs_in.Tex).r;
    float roughness = texture(texture_roughness1, fs_in.Tex).r;
    float ao = texture(texture_ao1, fs_in.Tex).r;
    vec3 emissive = texture(texture_emissive1, fs_in.Tex).rgb;

    vec3 normal = normalize(fs_in.Normal);
    
    gPosition = vec4(fs_in.FragPos, linearDepth(fs_in.FragPos));
    gNormal = vec4(normal, 1.0);
    gAlbedoSpec = vec4(albedo, specular);
    gMatProps = vec4(metallic, roughness, ao, 1.0);
    gEmissive = vec4(emissive, 1.0);
}