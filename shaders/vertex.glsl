
#version 330 core

layout(location=0) in vec3 pos;
layout(location=1) in vec3 color;
layout(location=2) in vec2 texCoord;

out vec3 vColor;
out vec2 vTexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

void main() {
    gl_Position = proj * view * model * vec4(pos, 1.0);
    vColor = color;
    vTexCoord = texCoord;
}
