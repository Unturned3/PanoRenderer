
#version 330 core

in vec4 localPos;
out vec4 fragColor;

uniform mat4 view;
uniform mat4 proj;
uniform sampler2D tex;
uniform float v_norm;
uniform float v_bias;
uniform float lod;

#define pi 3.141592653589793
const vec2 norm = vec2(0.5 / pi, 1.0 / pi);

vec2 Cart2Spherical(vec3 p) {
    /*
    vec2 uv = vec2(atan(p.x, -p.z), asin(p.y));
    return uv * norm + vec2(0.5);
    */
    /*
        atan(p.x, -p.z): this is because we're trying to find the azimuth
        angle between the point and the negative z-axis, in accordance with
        the OpenGL convention that the camera faces negative z.
    */
    float u = atan(p.x, -p.z) / (2 * pi);
    float v = asin(p.y) * v_norm + v_bias;
    float ua = u + 0.5;
    float ub = fract(u + 1.0) - 0.5;
    // 1-v, so we don't have to flip frames when loading them from disk.
    // Actually, we use v here now to render the images upside-down.
    // See NOTE in Pose.hpp, line 37.
    return vec2(fwidth(ua) < fwidth(ub) ? ua : ub, v);
}

void main() {
    mat4 inv_view = inverse(view);
    mat4 inv_proj = inverse(proj);
    vec4 p = inv_view * inv_proj * localPos;
    vec2 uv = Cart2Spherical(normalize(p.xyz));
    fragColor = texture(tex, uv);
    //fragColor = textureLod(tex, uv, lod);
}
