#version 330 core
layout(location=0) out vec4 gPosition;      // xyz position, w = linear depth
layout(location=1) out vec4 gNormal;        // xyz normal, w = roughness
layout(location=2) out vec4 gAlbedoMetal;   // rgb albedo, a = metallic
layout(location=3) out vec4 gRoughAoEmiss;  // r=AO, g=emissive strength, b=unused, a=specular (optional)

in VS_OUT { vec3 FragPos; vec3 Normal; vec2 Tex; } fs_in;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;
uniform sampler2D texture_metallic1;
uniform sampler2D texture_roughness1;
uniform sampler2D texture_ao1;
uniform sampler2D texture_emissive1;

uniform float uTime;
uniform bool hasEmissive; // only animate when a real emissive map exists

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
    float metallic = texture(texture_metallic1, fs_in.Tex).r;
    float roughness = texture(texture_roughness1, fs_in.Tex).r;
    float ao = texture(texture_ao1, fs_in.Tex).r;
    vec3 emissive = texture(texture_emissive1, fs_in.Tex).rgb;

    // Only animate emissive if present; otherwise keep at 0
    float baseEmiss = (emissive.r + emissive.g + emissive.b) / 3.0;
    float flicker = 0.85 + 0.15 * sin(uTime * 10.0); // subtler
    float emissiveStrength = hasEmissive ? baseEmiss * flicker : 0.0;

    gPosition = vec4(fs_in.FragPos, linearDepth(fs_in.FragPos));
    gNormal   = vec4(encodeNormal(normalize(fs_in.Normal)), clamp(roughness, 0.04, 1.0), metallic);
    gAlbedoMetal = vec4(albedo, metallic);
    // pack: r=AO, g=emissive strength, b=unused, a=specular scalar
    gRoughAoEmiss = vec4(ao, emissiveStrength, 0.0, texture(texture_specular1, fs_in.Tex).r);
}