
#version 330 core

in vec4 localPos;
out vec4 fragColor;

uniform mat4 view;
uniform mat4 proj;
uniform sampler2D tex;
uniform vec2 norm;
uniform vec2 bias;

vec2 Cart2Spherical(vec3 v) {
    vec2 uv = vec2(atan(-v.x, v.z), asin(v.y));
    uv = uv * norm + bias;
    return uv;
}

void main() {
    mat4 inv_view = inverse(view);
    mat4 inv_proj = inverse(proj);
    vec4 p = inv_view * inv_proj * localPos;
    vec2 uv = Cart2Spherical(normalize(p.xyz));
    vec3 color = texture(tex, uv).rgb;
    fragColor = vec4(color, 1.0);
}
