
#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/random.hpp>

#include "AppState.hpp"

void updatePose()
{
    AppState& s = AppState::get();
    assert(s.poses.has_value());

    cnpy::NpyArray& arr = s.poses.value();
    double* data = arr.data<double>();

    if (s.enable_trajectory) {
        double *p = data + static_cast<size_t>(s.pose_idx) * arr.shape[1];

        /*  NOTE: traj.py uses X forward, Y right, Z down for the camera.
            However, we are using -Z forward, X right, Y up for the camera.
            So, in order to get the same rendering effect as the ones
            produced by traj.py, we will negate the yaw values here.

            NOTE: screw it. We're gonna use this OpenGL program's setup as the
            convention for rendering things.
        */
        float yaw = static_cast<float>(p[0]);  // pan

        /*  NOTE: OpenGL assumes bottom-left texture origin, while most other
            libraries (e.g. OpenCV) assumes upper-left. So, the frames loaded
            by OpenCV are upside-down when rendered in OpenGL. Previously, we
            render the frames correctly oriented to the window by changing every
            texture v coordinate to (1-v). However, this means the frames are
            "flipped" from the perspective of cv::VideoWriter when we read it
            back from the window with glReadPixels.

            So, for the sake of video codec efficiency, we don't do any frame
            flipping (at the cost of rendering them upside-down to the window).
            However, we also need to negate the pitch values correspondingly,
            as done below.
        */
        float pitch = -1 * static_cast<float>(p[1]);     // tilt
        float roll = static_cast<float>(p[2]);
        float fov = static_cast<float>(p[3]);

        s.pan = yaw, s.tilt = pitch, s.roll = roll;
        s.fov = fov;

        glm::mat4 R {1.0f};
        /*  NOTE: instead of yawing around the global Y after pitching,
            we can simply yaw before pitching when the local/global Y are
            still coincident.
        */
        R = glm::rotate(R, glm::radians(yaw), {0, 1, 0});
        R = glm::rotate(R, glm::radians(pitch), {1, 0, 0});
        // R = glm::rotate(R, glm::radians(yaw), glm::vec3(glm::row(R, 1)));
        R = glm::rotate(R, glm::radians(roll), {0, 0, -1});
        s.M_rot = R;
    }
}
