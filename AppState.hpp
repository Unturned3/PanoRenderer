
#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/random.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

class AppState {
public:
    static AppState& get()
    {
        static AppState s;
        return s;
    }

    float fov = 75.0f;
    float max_fov = 120.0f;
    bool showUI = true;
    bool randomTrajectory = false;

    glm::mat4 M_rot {1.0f};
    glm::vec3 front {0, 0, -1};
    glm::vec3 up {0};
    glm::vec3 right {0};

    AppState(const AppState& o) = delete;
    AppState& operator=(const AppState& o) = delete;
    AppState(AppState&& o) = delete;

private:
    AppState() {};
};
