#version 330 core
layout(location=0) out vec3 gPosition;      // world position
layout(location=1) out vec4 gNormalRough;   // RG: octahedral normal, B: roughness, A: metallic
layout(location=2) out vec4 gAlbedoMetal;   // RGB: albedo, A: metallic (duplicate for easy access)
layout(location=3) out vec4 gAoEmissSpec;   // R: AO, G: emissive, B: unused, A: specular

in VS_OUT { vec3 FragPos; vec3 Normal; vec2 Tex; } fs_in;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;
uniform sampler2D texture_metallic1;
uniform sampler2D texture_roughness1;
uniform sampler2D texture_ao1;
uniform sampler2D texture_emissive1;

uniform float uTime;
uniform bool hasEmissive;

// Octahedral encoding for normals (2 components instead of 3)
vec2 encodeNormal(vec3 n) {
    n /= (abs(n.x) + abs(n.y) + abs(n.z));
    return n.z >= 0.0 ? n.xy : (1.0 - abs(n.yx)) * sign(n.xy);
}

void main() {
    gPosition = fs_in.FragPos;

    vec3 albedo = texture(texture_diffuse1, fs_in.Tex).rgb;
    float metallic = texture(texture_metallic1, fs_in.Tex).r;
    float roughness = texture(texture_roughness1, fs_in.Tex).r;
    float ao = texture(texture_ao1, fs_in.Tex).r;
    vec3 emissive = texture(texture_emissive1, fs_in.Tex).rgb;

    float baseEmiss = (emissive.r + emissive.g + emissive.b) / 3.0;
    float flicker = 0.85 + 0.15 * sin(uTime * 10.0);
    float emissiveStrength = hasEmissive ? baseEmiss * flicker : 0.0;

    vec2 encodedNormal = encodeNormal(normalize(fs_in.Normal));
    gNormalRough = vec4(encodedNormal, clamp(roughness, 0.04, 1.0), metallic);
    gAlbedoMetal = vec4(albedo, metallic);
    gAoEmissSpec = vec4(ao, emissiveStrength, 0.0, texture(texture_specular1, fs_in.Tex).r);
}