
#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/random.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/quaternion.hpp>

#include "AppState.hpp"
#include "Window.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

class GUI {
public:
    GUI(InteractiveGLContext& window)
        : s(AppState::get()), window_(window), io_(ImGuiEarlyInit())
    {
        io_.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        ImGui::StyleColorsDark();

        ImGui_ImplGlfw_InitForOpenGL(window_.get(), true);
        ImGui_ImplOpenGL3_Init("#version 150");
    }

    ~GUI()
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }

    GUI(const GUI& o) = delete;
    GUI& operator=(const GUI& o) = delete;

    void update()
    {
        updateFps();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin(
            "Debug Info (Press H to hide/show)", nullptr,
            ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoScrollbar);

        float pan = atan2f(s.M_rot[2][0], s.M_rot[2][2]);
        float tilt = atan2f(-s.M_rot[2][1], sqrtf(powf(s.M_rot[0][1], 2) + powf(s.M_rot[1][1], 2)));
        float roll = atan2f(s.M_rot[0][1], s.M_rot[1][1]);
        ImGui::Text("R Pan: %5.1f°, Tilt: %5.1f°, Roll: %4.1f°, FoV: %3.1f°",
            glm::degrees(pan), glm::degrees(tilt), glm::degrees(roll), s.fov);
        ImGui::Text("E Pan: %5.1f°, Tilt: %5.1f°, Roll: %4.1f°, FoV: %4.1f°",
            s.pan, s.tilt, s.roll, s.fov);

        ImGui::Text("Average %.2f ms/frame (%.1f FPS)", 1000.0f / s.fps, s.fps);
        // ImGui::Text("Window focused: %s", ImGui::IsWindowFocused());
        ImGui::Text("M_rot: ");
        ImGui::SameLine();
        ImGui::Text(
            "%s",
            utils::pretty_matrix(glm::value_ptr(s.M_rot), 4, 4, 2).c_str());

        ImGui::Text("M_proj: ");
        ImGui::SameLine();
        ImGui::Text(
            "%s",
            utils::pretty_matrix(glm::value_ptr(s.M_proj), 4, 4, 2).c_str());

        ImGui::Text("|right|: %f", glm::length(s.right));
        ImGui::Checkbox("Enable trajectory", &s.enable_trajectory);
        if (ImGui::Button("Reset trajectory")) {
            s.pose_idx = 0;
        }
        ImGui::End();
    }

    void render()
    {
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

public:
    AppState& s;
    InteractiveGLContext& window_;
    ImGuiIO& io_;

    static ImGuiIO& ImGuiEarlyInit()
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        return ImGui::GetIO();
    }

    void updateFps()
    {
        s.frame_cnt++;
        s.fps_sum += io_.Framerate;
        int limit = (int)s.fps / 10 + 1;
        if (s.frame_cnt == limit) {
            s.fps = s.fps_sum / (float)limit;
            s.frame_cnt = 0;
            s.fps_sum = 0;
        }
    }
};
