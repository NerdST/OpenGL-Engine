#version 330 core
out vec4 FragColor;
in vec2 Tex;

struct PointLight {
    vec3 position;
    vec3 color;
    float radius;
};

uniform int uPointLightCount;
uniform PointLight uPointLights[32];
uniform vec3 viewPos;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoMetal;
uniform sampler2D gRoughAoEmiss;
uniform sampler2D ssaoTex;

const float PI = 3.14159265359;

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
    float ggx2 = GeometrySchlickGGX(NdotV,rough);
    float ggx1 = GeometrySchlickGGX(NdotL,rough);
    return ggx1*ggx2;
}

void main(){
    vec3 pos = texture(gPosition, Tex).xyz;
    vec3 N = normalize(texture(gNormal, Tex).xyz);
    vec4 albedoMetal = texture(gAlbedoMetal, Tex);
    vec4 pack = texture(gRoughAoEmiss, Tex);
    vec3 albedo = pow(albedoMetal.rgb, vec3(2.2)); // gamma to linear
    float metallic = albedoMetal.a;
    float roughness = texture(gNormal, Tex).w;
    float ao = pack.r * texture(ssaoTex, Tex).r;
    float emiss = pack.g;
    float specScalar = pack.a;
    vec3 V = normalize(viewPos - pos);
    vec3 F0 = mix(vec3(0.04*specScalar), albedo, metallic);

    vec3 Lo = vec3(0.0);
    for(int i=0;i<uPointLightCount;i++){
        PointLight Lgt = uPointLights[i];
        vec3 L = normalize(Lgt.position - pos);
        vec3 H = normalize(V + L);
        float dist = length(Lgt.position - pos);
        float attenuation = 1.0 / (1.0 + dist*dist / (Lgt.radius*Lgt.radius));
        float NdotL = max(dot(N,L),0.0);

        float D = DistributionGGX(N,H,roughness);
        float G = GeometrySmith(N,V,L,roughness);
        vec3  F = fresnelSchlick(max(dot(H,V),0.0), F0);

        vec3 numerator = D*G*F;
        float denom = 4.0*max(dot(N,V),0.0)*NdotL + 0.001;
        vec3 specular = numerator / denom;

        vec3 kS = F;
        vec3 kD = (vec3(1.0)-kS)*(1.0 - metallic);

        vec3 irradiance = Lgt.color * attenuation * NdotL;
        Lo += (kD*albedo/PI + specular) * irradiance;
    }
    vec3 ambient = albedo * ao * 0.01; // slightly darker baseline

    vec3 color = ambient + Lo + vec3(emiss);
    color = pow(color, vec3(1.0/2.2));
    FragColor = vec4(color,1.0);
}