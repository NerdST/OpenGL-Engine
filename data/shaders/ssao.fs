#version 330 core
out float FragAO;
in vec2 Tex;
uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D noiseTex;
uniform vec3 samples[64];
uniform mat4 projection;
uniform float radius;
uniform float bias;
void main(){
    vec3 pos = texture(gPosition, Tex).xyz;
    vec3 normal = normalize(texture(gNormal, Tex).xyz);
    vec3 randomVec = normalize(texture(noiseTex, Tex * 4.0).xyz);
    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);

    float occlusion = 0.0;
    for(int i=0;i<64;i++){
        vec3 samplePos = TBN * samples[i];
        samplePos = pos + samplePos * radius;

        vec4 offset = projection * vec4(samplePos,1.0);
        offset.xyz /= offset.w;
        offset.xyz = offset.xyz * 0.5 + 0.5;

        float sampleDepth = texture(gPosition, offset.xy).w;
        float rangeCheck = smoothstep(0.0,1.0,radius / abs(pos.z - samplePos.z));
        occlusion += (sampleDepth >= samplePos.z + bias ? 1.0 : 0.0) * rangeCheck;
    }
    occlusion = 1.0 - (occlusion / 64.0);
    FragAO = occlusion;
}