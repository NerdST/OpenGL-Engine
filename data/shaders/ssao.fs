#version 330 core
out float FragAO;
in vec2 Tex;

uniform sampler2D gNormalRough;  // RG: oct normal, B: rough, A: metal
uniform sampler2D depthTex;      // depth from GBuffer
uniform sampler2D noiseTex;
uniform vec3 samples[64];
uniform mat4 projection;
uniform mat4 invProjection;
uniform mat4 invView;
uniform float radius;
uniform float bias;

// Decode octahedral normal
vec3 decodeNormal(vec2 enc) {
    vec3 n;
    n.z = 1.0 - abs(enc.x) - abs(enc.y);
    n.xy = n.z >= 0.0 ? enc.xy : (1.0 - abs(enc.yx)) * sign(enc.xy);
    return normalize(n);
}
vec3 reconstructPosition(vec2 texCoord, float depth) {
    vec4 clipSpace = vec4(texCoord * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec4 viewSpace = invProjection * clipSpace;
    viewSpace /= viewSpace.w;
    vec4 worldSpace = invView * viewSpace;
    return worldSpace.xyz;
}

void main(){
    float depth = texture(depthTex, Tex).r;
    if(depth >= 0.9999){ FragAO = 1.0; return; }  // sky/background
    vec3 pos = reconstructPosition(Tex, depth);
    vec3 normal = decodeNormal(texture(gNormalRough, Tex).rg);

    vec3 randomVec = normalize(texture(noiseTex, Tex * 4.0).xyz);
    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);

    float occlusion = 0.0;
    for(int i=0;i<64;i++){
        vec3 samplePos = pos + TBN * samples[i] * radius;

        vec4 offset = projection * vec4(samplePos,1.0);
        offset.xyz /= offset.w;
        offset.xyz = offset.xyz * 0.5 + 0.5;

        float sampleDepth = texture(depthTex, offset.xy).r;
        float rangeCheck = smoothstep(0.0,1.0, radius / abs(pos.z - samplePos.z));
        occlusion += (sampleDepth >= offset.z + bias ? 1.0 : 0.0) * rangeCheck;
    }
    FragAO = 1.0 - (occlusion / 64.0);
}