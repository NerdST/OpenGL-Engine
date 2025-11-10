#version 330 core
out float FragAO;
in vec2 Tex;
uniform sampler2D ssaoInput;
void main(){
    float result = 0.0;
    float w[9] = float[](1,2,3,2,6,2,3,2,1);
    ivec2 size = textureSize(ssaoInput,0);
    for(int x=-1;x<=1;x++)
    for(int y=-1;y<=1;y++){
        vec2 uv = Tex + vec2(x,y)/vec2(size);
        result += texture(ssaoInput, uv).r * w[(x+1)*3 + (y+1)];
    }
    FragAO = result / 22.0;
}