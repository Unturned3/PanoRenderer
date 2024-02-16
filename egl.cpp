
#include <EGL/egl.h>

#include "fmt/core.h"
#include "utils.hpp"

// clang-format off
static const EGLint eglConfigAttrs[] = {
    EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
    EGL_BLUE_SIZE, 8,
    EGL_GREEN_SIZE, 8,
    EGL_RED_SIZE, 8,
    EGL_DEPTH_SIZE, 8,
    EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
    EGL_NONE,
};
// clang-format on

static const int pbufferWidth = 9;
static const int pbufferHeight = 9;

static const EGLint pbufAttrs[] = {
    EGL_WIDTH, pbufferWidth, EGL_HEIGHT, pbufferHeight, EGL_NONE,
};

int main(int argc, char *argv[])
{
    // 1. Initialize EGL
    EGLDisplay eglDpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);

    EGLint major, minor;

    eglInitialize(eglDpy, &major, &minor);
    LOG(fmt::format("EGL version: {}.{}", major, minor));

    // 2. Select an appropriate configuration
    EGLint numConfigs;
    EGLConfig eglCfg;

    eglChooseConfig(eglDpy, eglConfigAttrs, &eglCfg, 1, &numConfigs);

    // 3. Create a surface
    EGLSurface eglSurf =
        eglCreatePbufferSurface(eglDpy, eglCfg, pbufAttrs);

    // 4. Bind the API
    eglBindAPI(EGL_OPENGL_API);

    // 5. Create a context and make it current
    EGLContext eglCtx =
        eglCreateContext(eglDpy, eglCfg, EGL_NO_CONTEXT, nullptr);

    eglMakeCurrent(eglDpy, eglSurf, eglSurf, eglCtx);

    // from now on use your OpenGL context

    // 6. Terminate EGL when finished
    eglTerminate(eglDpy);
    return 0;
}