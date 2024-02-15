
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
    float fov = 75.0f;
    float max_fov = 120.0f;
    bool drawUI = true;
    bool randomPose = false;

    glm::mat4 M_rot {1.0f};
    glm::vec3 front {0, 0, -1};
    glm::vec3 up {0};
    glm::vec3 right {0};

    int frame_cnt = 0;
    float fps_sum = 0;
    float fps = 1;

    // Random pose generation parameters
    float focal_accel_m = 5;
    float pose_accel_m = 3;
    float delta = 1.0f / 60.0f;
    float max_vel = 1;

    float mean_pitch_accel = 0;
    float mean_focal_accel = 0;

    glm::vec3 accel {0};
    glm::vec3 vel {0};
    glm::vec3 pose {0};

    glm::vec3 prev_pose {0, 0, 75};
    glm::vec3 prev_vel {0, 0, 0};

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
