
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

        /*  NOTE: cv2 imread, imwrite, VideoCapture, and VideoWriter all
            assumes a top-left texture origin. When they load a correctly
            oriented image from disk, the top-left corner of the image is (0,0).
            When they write an image to disk, they assume the input image's
            origin is at the top-left.

            However, OpenGL uses bottom-left as the texture origin. So, it
            will display an upright image loaded by cv2 with the y-axis flipped.
            Similarly, if an upright image in OpenGL's framebuffer is written to
            disk using cv2, it will be flipped in the y-axis again.

            Flipping images on the CPU is slow. Therefore, we never flip them,
            and OpenGL will work with the y-flipped images. However, this means
            that we need to negate our pitch and roll angles. Or else, pitching up in
            OpenGL will actually be pitching down in the final output video (and
            rolling clockwise in-screen will appear as a counter-clockwise
            rotation). */

        float yaw = static_cast<float>(p[0]);  // pan
        float pitch = -1 * static_cast<float>(p[1]);     // tilt
        float roll = -1 * static_cast<float>(p[2]);
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
        R = glm::rotate(R, glm::radians(roll), {0, 0, 1});
        s.M_rot = R;
    }
}
