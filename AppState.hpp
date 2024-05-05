
#pragma once

#include <optional>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/random.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include "cnpy.h"

class AppState {
public:
    bool drawUI = true;

    std::optional<cnpy::NpyArray> poses = std::nullopt;

    // TODO: make this explicitly specify hfov or vfov
    float pan = 0, tilt = 0, roll = 0;
    float fov = 75.0f;
    glm::mat4 M_rot {1.0f};

    glm::mat4 M_proj {1.0f};
    glm::vec3 front {0, 0, -1};
    glm::vec3 up {0};
    glm::vec3 right {0};

    bool enable_trajectory = true;
    int pose_idx = 0;
    int frame_cnt = 0;
    float fps_sum = 0;
    float fps = 1;

    float max_fov = 120.0f;

public:
    static AppState& get()
    {
        static AppState s;
        return s;
    }

    AppState(const AppState& o) = delete;
    AppState& operator=(const AppState& o) = delete;
    AppState(AppState&& o) = delete;

private:
    AppState() {};
};
