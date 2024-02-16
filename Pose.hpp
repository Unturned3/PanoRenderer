
#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/random.hpp>

#include "AppState.hpp"

void updatePose()
{
    AppState& s = AppState::get();

    // Calculate trajectory update
    glm::vec2 pitch_yaw_accel {
        glm::gaussRand(s.mean_pitch_accel, 1.0f) / 1.5,  // pitch
        glm::gaussRand(0.0f, 1.0f),                      // yaw
    };
    s.accel = {
        glm::normalize(pitch_yaw_accel) * s.pose_accel_m,
        glm::gaussRand(s.mean_focal_accel, 1.0f) * s.focal_accel_m,  // focal
    };

    // Update trajectory
    s.vel = s.prev_vel + s.accel * s.delta;

    if (glm::length(s.vel) > s.max_vel)
        s.vel = s.max_vel * glm::normalize(s.vel);

    s.pose = s.prev_pose + s.vel * s.delta;

    s.mean_pitch_accel = -s.pose.x / 2;
    s.mean_focal_accel = (55 - s.pose.z) / 35;

    s.prev_pose = s.pose;
    s.prev_vel = s.vel;

    if (s.randomPose) {
        s.up = glm::vec3(glm::row(s.M_rot, 1));
        s.right = glm::cross(s.front, s.up);
        glm::vec3 right_ = glm::normalize(s.right);

        // M_rot = glm::rotate(M_rot, glm::radians(-rot_a), front);
        s.M_rot = glm::rotate(s.M_rot, glm::radians(s.vel.x), right_);
        s.M_rot = glm::rotate(s.M_rot, glm::radians(s.vel.y), s.up);
    }
}
