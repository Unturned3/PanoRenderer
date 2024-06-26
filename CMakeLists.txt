
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
add_subdirectory(cnpy)

set(GL_components OpenGL)

if(${OS_} STREQUAL Linux)
    set(OpenGL_GL_PREFERENCE GLVND)
    list(APPEND GL_components EGL GLX)
endif()

find_package(OpenGL REQUIRED COMPONENTS ${GL_components})
find_package(GLEW REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)
find_package(ZLIB REQUIRED)
find_package(OpenCV REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBAV REQUIRED IMPORTED_TARGET
    #libavdevice
    #libavfilter
    libavformat
    libavcodec
    #libswresample
    #libswscale
    libavutil
)


### Main executable ###

add_executable(main main.cpp)

if(${OS_} STREQUAL Linux)
    set(USE_EGL TRUE)
endif()

set_target_properties(main PROPERTIES CXX_EXTENSIONS OFF)
target_compile_features(main PRIVATE cxx_std_20)
target_compile_options(main PRIVATE
    -Wall -Wextra -Wpedantic -Wconversion
    -Wno-unused-variable -Wno-unused-parameter
    -O3 #-g3 -Werror
)
if(${USE_EGL})
    target_compile_definitions(main PRIVATE USE_EGL)
endif()

target_link_libraries(main PRIVATE
    stb_image
    imgui
    fmt::fmt
    OpenGL::GL
    glfw
    glm::glm
    cnpy
    ZLIB::ZLIB
    ${OpenCV_LIBS}
)

if(${USE_EGL})
    target_link_libraries(main PRIVATE
        OpenGL::EGL
        ${CMAKE_SOURCE_DIR}/libGLEW.a
    )
else()
    target_link_libraries(main PRIVATE GLEW::glew)
endif()


# video decoding/encoding test executable

add_executable(vcd vcodec_test.cpp)

set_target_properties(vcd PROPERTIES CXX_EXTENSIONS OFF)
target_compile_features(vcd PRIVATE cxx_std_20)
target_compile_options(vcd PRIVATE
    -Wall -Wextra -Wpedantic -Wconversion
    -Wno-unused-variable -Wno-unused-parameter
    -Wno-c99-extensions
    -O3 #-g3 -Werror
)
target_link_libraries(vcd PRIVATE
    PkgConfig::LIBAV fmt::fmt stb_image
    ${OpenCV_LIBS}
)
