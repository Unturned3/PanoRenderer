
#version 330 core

layout(location=0) in vec3 pos;

out vec4 localPos;

void main() {
    localPos = vec4(pos, 1.0);
    gl_Position = localPos;
}
