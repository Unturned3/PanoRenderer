
#version 330 core

in vec3 vColor;
in vec2 vTexCoord;

out vec4 fragColor;

uniform bool useTexture;
uniform vec3 color;
uniform sampler2D tex;

void main() {
    if (useTexture) {
        fragColor = texture(tex, vTexCoord);
    } else {
        fragColor = vec4(color, 1.0);
    }
}
