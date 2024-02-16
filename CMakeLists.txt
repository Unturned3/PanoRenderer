
cmake_minimum_required(VERSION 3.21...3.28)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(OpenGL-Test VERSION 0.1
                    DESCRIPTION "OpenGL Test"
                    LANGUAGES C CXX)

# Include custom modules
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
include(utils)

prevent_in_src_build()

# Get info about build environment
get_os_name(OS_)
get_linux_distro_name(DISTRO_)


add_subdirectory(stb_image)
add_subdirectory(imgui)
add_subdirectory(fmt)

set(GL_components OpenGL)

if(${OS_} STREQUAL Linux)
    set(OpenGL_GL_PREFERENCE GLVND)
    list(APPEND GL_components EGL GLX)
endif()

find_package(OpenGL REQUIRED COMPONENTS ${GL_components})
find_package(GLEW REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)


### Main executable ###

add_executable(main main.cpp)

set_target_properties(main PROPERTIES CXX_EXTENSIONS OFF)
target_compile_features(main PRIVATE cxx_std_20)
target_compile_options(main PRIVATE
    -Wall -Wextra -Wpedantic -Wconversion
    -Wno-unused-variable -Wno-unused-parameter
    -O3 #-g3 -Werror
)

target_link_libraries(main PRIVATE
    stb_image
    imgui
    fmt::fmt
    OpenGL::GL
    GLEW::glew
    glfw
    glm::glm
)


### EGL executable (Linux only) ###

if(${OS_} STREQUAL Linux)

    add_executable(egl egl.cpp)

    target_link_libraries(egl PRIVATE
        stb_image
        imgui
        fmt::fmt
        OpenGL::EGL
    )

    # Use GLEW built with EGL support
    target_link_libraries(egl PRIVATE ${CMAKE_SOURCE_DIR}/glew-2.2.0/lib/libGLEW.a)

    set_target_properties(egl PROPERTIES CXX_EXTENSIONS OFF)
    target_compile_features(egl PRIVATE cxx_std_20)
    target_compile_options(egl PRIVATE
        -Wall -Wextra -Wpedantic -Wconversion
        -Wno-unused-variable -Wno-unused-parameter
        -O3 #-g3 -Werror
    )

endif()


#if(${DISTRO_} STREQUAL arch)
#    target_link_libraries(main PRIVATE glm)
#else()
#    target_link_libraries(main PRIVATE glm::glm)
#endif()
