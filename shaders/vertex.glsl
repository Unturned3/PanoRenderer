
#version 330 core

layout(location=0) in vec2 pos;
layout(location=1) in vec3 color;
layout(location=2) in vec2 texCoord;

out vec3 vColor;
out vec2 vTexCoord;

void main() {
    gl_Position = vec4(pos, 0.0, 1.0);
    vColor = color;
    vTexCoord = texCoord;
}
