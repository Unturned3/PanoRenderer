
#include "Sphere.hpp"

using namespace glm;

std::vector<vec3> subdivide_triangle(std::vector<vec3>& vertices)
{
    std::vector<vec3> t(vertices.size() * 4);

    for (size_t i = 0; i < vertices.size(); i += 3) {
        vec3 a = vertices[i];
        vec3 b = vertices[i+1];
        vec3 c = vertices[i+2];

        vec3 ab = (a + b) / 2.0f;
        vec3 bc = (b + c) / 2.0f;
        vec3 ca = (c + a) / 2.0f;

        t.push_back(a);
        t.push_back(ab);
        t.push_back(ca);

        t.push_back(ab);
        t.push_back(bc);
        t.push_back(ca);

        t.push_back(ab);
        t.push_back(b);
        t.push_back(bc);

        t.push_back(ca);
        t.push_back(bc);
        t.push_back(c);
    }

    return t;
}
