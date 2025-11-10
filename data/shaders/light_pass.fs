#version 330 core
out vec4 FragColor;

uniform vec3 lightPos;
uniform vec3 lightColor;
uniform float lightRadius;
uniform vec3 viewPos;
uniform mat4 invProjection;
uniform mat4 invView;

uniform sampler2D gNormalRough;
uniform sampler2D gAlbedoMetal;
uniform sampler2D gAoEmissSpec;
uniform sampler2D depthTex;
uniform sampler2D ssaoTex;

const float PI = 3.14159265359;

// Decode octahedral normal
vec3 decodeNormal(vec2 enc) {
    vec3 n;
    n.z = 1.0 - abs(enc.x) - abs(enc.y);
    n.xy = n.z >= 0.0 ? enc.xy : (1.0 - abs(enc.yx)) * sign(enc.xy);
    return normalize(n);
}

// Reconstruct world position from depth
vec3 reconstructPosition(vec2 texCoord, float depth) {
    vec4 clipSpace = vec4(texCoord * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec4 viewSpace = invProjection * clipSpace;
    viewSpace /= viewSpace.w;
    vec4 worldSpace = invView * viewSpace;
    return worldSpace.xyz;
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

void main(){
    vec2 texCoord = gl_FragCoord.xy / vec2(1280, 720);
    
    // Read packed data
    vec4 normalRough = texture(gNormalRough, texCoord);
    vec4 albedoMetal = texture(gAlbedoMetal, texCoord);
    vec4 aoEmissSpec = texture(gAoEmissSpec, texCoord);
    float depth = texture(depthTex, texCoord).r;
    
    // Early discard for skybox
    if (depth >= 0.9999) discard;
    
    // Decode data
    vec3 N = decodeNormal(normalRough.xy);
    float roughness = normalRough.z;
    float metallic = normalRough.w;
    vec3 albedo = pow(albedoMetal.rgb, vec3(2.2));
    float ao = aoEmissSpec.r * texture(ssaoTex, texCoord).r;
    float specScalar = aoEmissSpec.a;
    
    vec3 pos = reconstructPosition(texCoord, depth);
    vec3 V = normalize(viewPos - pos);
    vec3 F0 = mix(vec3(0.04*specScalar), albedo, metallic);
    
    // Calculate this single light
    vec3 L = normalize(lightPos - pos);
    vec3 H = normalize(V + L);
    float dist = length(lightPos - pos);
    float attenuation = 1.0 / (1.0 + dist*dist / (lightRadius*lightRadius));
    
    // Early exit if outside light range
    if (attenuation < 0.001) discard;
    
    float NdotL = max(dot(N,L),0.0);
    
    float D = DistributionGGX(N,H,roughness);
    float G = GeometrySmith(N,V,L,roughness);
    vec3  F = fresnelSchlick(max(dot(H,V),0.0), F0);
    
    vec3 numerator = D*G*F;
    float denom = 4.0*max(dot(N,V),0.0)*NdotL + 0.001;
    vec3 specular = numerator / denom;
    
    vec3 kS = F;
    vec3 kD = (vec3(1.0)-kS)*(1.0 - metallic);
    
    vec3 irradiance = lightColor * attenuation * NdotL;
    vec3 Lo = (kD*albedo/PI + specular) * irradiance;
    
    FragColor = vec4(Lo, 1.0);
}