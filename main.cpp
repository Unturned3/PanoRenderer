
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <cassert>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/random.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include <string>
#include <vector>

#include <opencv2/opencv.hpp>

#include "AppState.hpp"
#include "GUI.hpp"
#include "Image.hpp"
#include "Pose.hpp"
#include "Shader.hpp"
#include "VertexBuffer.hpp"
#include "Window.hpp"
#include "utils.hpp"
#include "cnpy.h"
#include "PanoContainer.hpp"

static void glErrorCallback_(GLenum source, GLenum type, GLuint id,
                             GLenum severity, GLsizei length, const GLchar* msg,
                             const void* userParam)
{
    LOG("OpenGL error:");
    LOG(msg);
}

int main(int argc, char** argv)
{
#ifdef USE_EGL
    HeadlessGLContext window(640, 480, "OpenGL Test");
#else
    InteractiveGLContext window(640, 480, "OpenGL Test");
#endif

    std::string panoFilePath = argc < 2 ? "../images/p1.jpg" : argv[1];

    PanoContainer pano;

    int fourcc = cv::VideoWriter::fourcc('a', 'v', 'c', '1');
    cv::VideoWriter videoWriter("out.mp4", fourcc, 30, {640, 480}, true);
    check(videoWriter.isOpened(), "Error opening cv::VideoWriter");

    if (panoFilePath.substr(panoFilePath.length() - 4) == ".mp4") {
        pano = PanoContainer(cv::VideoCapture(panoFilePath, cv::CAP_FFMPEG));
    } else {
        pano = PanoContainer(Image(panoFilePath, false));
    }

    if (argc >= 3) {
        std::string posesFilePath {argv[2]};
        AppState::get().poses = cnpy::npy_load(posesFilePath);
    }

    if (glewInit() != GLEW_OK) throw std::runtime_error("GLEW init failed.");

    if (GLEW_KHR_debug)
        glDebugMessageCallback(glErrorCallback_, nullptr);
    else
        LOG("glDebugMessageCallback not available.");

#ifndef USE_EGL
    GUI gui(window);
#endif

    uint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        float borderColor[] = {0.2f, 0.2f, 0.2f, 1.0f};
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // TODO: use GL_BGR for video from opencv?
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, pano.width, pano.height, 0, GL_RGB,
                 GL_UNSIGNED_BYTE, pano.data);

    // Rectangle (two triangles) covering the screen.
    constexpr int stride = 3;
    // clang-format off
    std::vector<float> vertices {
        -1, -1,  0,
         1,  1,  0,
        -1,  1,  0,
        -1, -1,  0,
         1, -1,  0,
         1,  1,  0,
    };
    // clang-format on

    VertexBuffer vb(vertices.data(),
                    static_cast<uint>(vertices.size() * sizeof(float)));

    Shader shader(utils::path("shaders/vertex.glsl"),
                  utils::path("shaders/frag.glsl"));
    shader.use();
    {
        // proportion of the missing sphere that's below the horizon
        double m = 1;
        if (argc >= 4) m = std::stod(argv[3]);
        double u = (double)pano.width / 2 / (double)pano.height;
        float v_n = (float)(u * M_1_PI);
        float v_b = (float)(u / 2 + (1 - u) * m);
        shader.setFloat("v_norm", v_n);
        shader.setFloat("v_bias", v_b);
    }

    uint VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride * sizeof(float),
                          (void*)0);
    vb.bind();

    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

#ifndef USE_EGL
    // Desired FPS
    const double desiredFps = 30.0;
    const double desiredFrameTime = 1.0 / desiredFps;

    // Timing variables
    double lastTime = glfwGetTime();
    double currentTime = 0.0;
    double deltaTime = 0.0;
#endif

    while (!window.shouldClose()) {

#ifndef USE_EGL
        currentTime = glfwGetTime();
        deltaTime = currentTime - lastTime;
#endif

        AppState& s = AppState::get();

        if (s.poses.has_value()) {
            updatePose();
        }

#ifndef USE_EGL
        window.handleKeyDown();
#endif

        // Recalculate LoD, perspective, & view.
        // TODO: vary LoD calculation based on how close to edge a pixel is?
        // Need to check if this is actually a problem. If no visible aliasing,
        // then ignore.
        float fov_thresh = 75.0f;
        if (s.fov <= fov_thresh)
            shader.setFloat("lod", 0.0f);
        else
            shader.setFloat(
                "lod", (s.fov - fov_thresh) / (s.max_fov - fov_thresh) + 1.0f);

        // TODO: glm::perspective takes vfov, not hfov
        glm::mat4 M_proj = glm::perspective(glm::radians(s.fov),
                                            window.aspectRatio(), 0.1f, 2.0f);
        s.M_proj = M_proj;
        shader.setMat4("proj", M_proj);

        // NOTE: by convention, camera faces -Z, and Y is up, X is right.
        glm::mat4 M_view =
            glm::lookAt(glm::vec3(0), -glm::vec3(glm::column(s.M_rot, 2)),
                        glm::vec3(glm::column(s.M_rot, 1)));
        shader.setMat4("view", M_view);

        // Clear frame
        glClear(GL_COLOR_BUFFER_BIT);

        // Draw projected panorama
        glBindTexture(GL_TEXTURE_2D, tex);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0,
                     static_cast<int>(vertices.size() / stride));

#ifdef USE_EGL
        /*
        auto [w, h] = window.framebufferShape();
        Image frame(w, h, 3);
        glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, frame.data());
        std::string name = "out.jpg";
        frame.write(name);
        LOG("frame saved to " + name);
        */
        break;
#else
        if (s.drawUI) {
            gui.update();
            gui.render();
        }
        auto [w, h] = window.framebufferShape();
        cv::Mat mat(cv::Size(w, h), CV_8UC3);

        /*  NOTE: we tell OpenGL that everything is in RGB, while in reality
            the frames loaded by OpenCV is in BGR, and cv::VideoWriter expects
            BGR input. If OpenGL renders everything as RGB, the output frame
            in the window will have R-B swapped, but the encoded video will
            look correct.
        */
        {
            GLenum fmt = GL_RGB;
            if (!pano.isVideo) {
                /*  Unlike OpenCV, stbi_image loads images as RGB, so we need
                    to swap the channels to give cv::VideoWriter what it wants
                    i.e. BGR. */
                fmt = GL_BGR;
            }
            glReadPixels(0, 0, w, h, fmt, GL_UNSIGNED_BYTE, mat.data);
        }

        /*  On macOS, the FB shape might be larger than window shape due
            to the use of HiDPI displays, so we explicitly downscale the
            frame to the desired size.
        */
        cv::resize(mat, mat, {640, 480});
        videoWriter.write(mat);

        window.swapBuffers();
        // Control the frame rate
        /*
        while (deltaTime < desiredFrameTime) {
            // Wait to maintain frame rate
            currentTime = glfwGetTime();
            deltaTime = currentTime - lastTime;
        }
        lastTime = currentTime;
        */
#endif

        // Update frame as appropriate
        if (pano.isVideo) {
            pano.nextFrame();
            if (pano.frame.empty()) {
                break;
            }
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, pano.width, pano.height,
                GL_RGB, GL_UNSIGNED_BYTE, pano.data);
        }

        if (s.poses.has_value()) {
            s.pose_idx += 1;
            if (static_cast<size_t>(s.pose_idx) == s.poses.value().shape[0]) {
                break;
            }
        }
    }

    videoWriter.release();
    return 0;
}
