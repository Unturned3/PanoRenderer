
#version 330 core

in vec3 vColor;
in vec2 vTexCoord;
out vec4 fragColor;
uniform float tColor;
uniform sampler2D tex;

void main() {
    /*
    fragColor = vec4(vColor, 1.0);
    fragColor.g += tColor;
    fragColor = clamp(fragColor, 0, 1);
    */
    fragColor = texture(tex, vTexCoord);
}
