
#version 330 core

in vec3 vColor;
out vec4 fragColor;
uniform float tColor;

void main() {
    fragColor = vec4(vColor, 1.0);
    fragColor.g += tColor;
    fragColor = clamp(fragColor, 0, 1);
}
