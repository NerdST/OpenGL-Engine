#version 330 core
out vec4 FragColor;
in vec2 TexCoords;

struct PointLight {
    vec3 position;
    vec3 color;
    float intensity;
    float radius;
};

uniform int uPointLightCount;
uniform PointLight uPointLights[32];
uniform vec3 viewPos;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;
uniform sampler2D gMatProps;
uniform sampler2D gEmissive;
uniform bool hasEmissive;

const float PI = 3.14159265359;

const float emissiveStrength = 1.0;

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
    vec3 fragPos   = texture(gPosition, TexCoords).rgb;
    vec3 N         = normalize(texture(gNormal, TexCoords).xyz);
    vec3 albedo    = texture(gAlbedoSpec, TexCoords).rgb;
    float specular = texture(gAlbedoSpec, TexCoords).a;
    
    vec4 matProps  = texture(gMatProps, TexCoords);
    float metallic = matProps.r;
    float roughness= matProps.g;
    float ao       = matProps.b;

    vec3 V = normalize(viewPos - fragPos);
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    vec3 Lo = vec3(0.0);
    for(int i=0;i<uPointLightCount;i++){
        PointLight Lgt = uPointLights[i];
        vec3 L = normalize(Lgt.position - fragPos);
        vec3 H = normalize(V + L);
        float dist = length(Lgt.position - fragPos);
        float attenuation = 1.0 / (1.0 + (dist*dist)/(Lgt.radius*Lgt.radius));
        vec3 radiance = Lgt.color * Lgt.intensity * attenuation;

        float NdotL = max(dot(N,L),0.0);
        float D = DistributionGGX(N,H,roughness);
        float G = GeometrySmith(N,V,L,roughness);
        vec3  F = fresnelSchlick(max(dot(H,V),0.0), F0);

        vec3 numerator = D*G*F;
        float denom = 4.0*max(dot(N,V),0.0)*NdotL + 0.001;
        vec3 specularTerm = numerator / denom;

        vec3 kS = F;
        vec3 kD = (vec3(1.0)-kS) * (1.0 - metallic);

        Lo += (kD*albedo/PI + specularTerm) * radiance * NdotL;
    }

    vec3 emissiveColor = hasEmissive ? texture(gEmissive, TexCoords).rgb * emissiveStrength : vec3(0.0);
    vec3 ambient = albedo * ao * 0.03;
    vec3 color = ambient + Lo + emissiveColor;
    FragColor = vec4(color, 1.0);
}