
#version 330 core

in vec4 localPos;
out vec4 fragColor;

uniform mat4 view;
uniform mat4 inv_proj;
uniform sampler2D tex;

const vec2 norm = vec2(0.159154943, 0.318309886);
//const vec2 norm = vec2(0.159154943, 0.38197186);
const vec2 bias = vec2(0.5, 0.5);

vec2 Cart2Spherical(vec3 v) {
    vec2 uv = vec2(atan(-v.x, v.z), asin(v.y));
    uv = uv * norm + bias;
    uv.y = uv.y * 1.46044 - 0.46044;
    return uv;
}

void main() {
    mat4 inv_view = inverse(view);
    vec4 p = inv_view * inv_proj * localPos;
    vec2 uv = Cart2Spherical(normalize(p.xyz));
    vec3 color = texture(tex, uv).rgb;
    fragColor = vec4(color, 1.0);
}
