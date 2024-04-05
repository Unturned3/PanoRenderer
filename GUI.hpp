
#pragma once

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
        // ImGui::SetNextWindowSize(ImVec2(270, 80));

        ImGui::Begin(
            "Debug Info (Press H to hide/show)", nullptr,
            ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoScrollbar);
        ImGui::Text("Pitch: %.1f°, Yaw: %.1f°, FoV: %.0f°", 0.0f, 0.0f, s.fov);
        ImGui::Text("Average %.2f ms/frame (%.1f FPS)", 1000.0f / s.fps, s.fps);
        // ImGui::Text("Window focused: %s", ImGui::IsWindowFocused());
        ImGui::Text("M_rot: ");
        ImGui::SameLine();
        ImGui::Text(
            "%s",
            utils::pretty_matrix(glm::value_ptr(s.M_rot), 4, 4, 2).c_str());
        ImGui::Text("|right|: %f", glm::length(s.right));
        ImGui::Text("accel: %s", glm::to_string(s.accel).c_str());
        ImGui::Text("vel: %s", glm::to_string(s.vel).c_str());
        ImGui::Text("pose: %s", glm::to_string(s.pose).c_str());

        ImGui::Checkbox("Enable random trajectory", &s.randomPose);

        if (ImGui::Button("Randomize rotation")) {
            float pitch = glm::linearRand(-50.0f, 50.0f);
            float yaw = glm::linearRand(-180.0f, 180.0f);
            glm::mat4 n {1.0f};
            n = glm::rotate(n, glm::radians(pitch), {1, 0, 0});
            n = glm::rotate(n, glm::radians(yaw), glm::vec3(glm::row(n, 1)));
            s.M_rot = n;
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
