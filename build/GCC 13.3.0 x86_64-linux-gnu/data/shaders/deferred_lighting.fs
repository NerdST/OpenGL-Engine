#version 330 core
out vec4 FragColor;
in vec2 Tex;

struct PointLight { vec3 position; vec3 color; float radius; };

uniform int uPointLightCount;
uniform PointLight uPointLights[32];
uniform vec3 viewPos;

uniform sampler2D gPosition;
uniform sampler2D gNormalRough;
uniform sampler2D gAlbedoMetal;
uniform sampler2D gAoEmissSpec;
uniform sampler2D ssaoTex;

// New: tweakables
uniform float uAmbient;        // e.g. 0.03
uniform float uSSAOStrength;   // e.g. 1.0

const float PI = 3.14159265359;

vec3 decodeNormal(vec2 enc) {
    vec3 n;
    n.z = 1.0 - abs(enc.x) - abs(enc.y);
    n.xy = n.z >= 0.0 ? enc.xy : (1.0 - abs(enc.yx)) * sign(enc.xy);
    return normalize(n);
}
vec3 reconstructPosition(vec2 texCoord, float depth) {
    // Will be passed via uniforms from CPU (set inv matrices there if needed)
    // For pure screen-space light, we can optionally supply inv matrices. Omitted here.
    return vec3(0.0); // not needed for fullscreen unless you want V from world pos; see below we avoid it
}

vec3 fresnelSchlick(float cosTheta, vec3 F0){
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}
float DistributionGGX(vec3 N, vec3 H, float rough){
    float a = rough*rough;
    float a2 = a*a;
    float NdotH = max(dot(N,H),0.0);
    float NdotH2 = NdotH*NdotH;
    float denom = (NdotH2*(a2-1.0)+1.0);
    return a2 / (PI * denom*denom);
}
float GeometrySchlickGGX(float NdotV,float rough){
    float r = rough + 1.0;
    float k = (r*r)/8.0;
    return NdotV / (NdotV*(1.0-k)+k);
}
float GeometrySmith(vec3 N, vec3 V, vec3 L, float rough){
    float NdotV = max(dot(N,V),0.0);
    float NdotL = max(dot(N,L),0.0);
    return GeometrySchlickGGX(NdotV,rough) * GeometrySchlickGGX(NdotL,rough);
}

void main() {
    vec2 uv = Tex;

    vec3 pos = texture(gPosition, uv).rgb;
    vec4 NR = texture(gNormalRough, uv);
    vec3 N = decodeNormal(NR.rg);
    float roughness = NR.b;
    float metallic  = NR.a;
    vec3 albedo = texture(gAlbedoMetal, uv).rgb; // treat as sRGB
    albedo = pow(albedo, vec3(2.2));             // to linear once
    vec4 AOE = texture(gAoEmissSpec, uv);
    float ao = mix(1.0, texture(ssaoTex, uv).r, uSSAOStrength);
    float emiss = AOE.g;
    float specScalar = AOE.a;

    vec3 V = normalize(viewPos - pos);
    vec3 F0 = mix(vec3(0.04 * specScalar), albedo, metallic);

    vec3 Lo = vec3(0.0);
    for(int i=0;i<uPointLightCount;i++){
        PointLight Lgt = uPointLights[i];
        vec3 Ldir = Lgt.position - pos;
        float dist = length(Ldir);
        if(dist > Lgt.radius) continue;
        vec3 L = normalize(Ldir);
        float attenuation = 1.0 - (dist / Lgt.radius); // simple falloff
        attenuation = attenuation * attenuation;

        float NdotL = max(dot(N,L),0.0);
        if(NdotL <= 0.0) continue;

        vec3 H = normalize(V + L);
        float D = DistributionGGX(N,H,roughness);
        float G = GeometrySmith(N,V,L,roughness);
        vec3 F = fresnelSchlick(max(dot(H,V),0.0), F0);

        vec3 numerator = D*G*F;
        float denom = 4.0 * max(dot(N,V),0.0) * NdotL + 0.001;
        vec3 specular = numerator / denom;

        vec3 kS = F;
        vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);

        vec3 irradiance = Lgt.color * attenuation * NdotL;
        Lo += (kD * albedo / PI + specular) * irradiance;
    }

    vec3 ambient = albedo * ao * uAmbient;  // <- ambient factor here
    vec3 color = ambient + Lo;
    color = pow(color, vec3(1.0/2.2)); // back to sRGB
    FragColor = vec4(color,1.0);
}