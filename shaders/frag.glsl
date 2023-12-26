
#version 330 core

in vec3 vColor;
in vec2 vTexCoord;

out vec4 fragColor;

uniform float tColor;
uniform sampler2D tex;

void main() {
    fragColor = texture(tex, vTexCoord);
}
