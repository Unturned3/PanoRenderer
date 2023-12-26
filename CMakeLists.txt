
cmake_minimum_required(VERSION 3.7...3.27)

project(OpenGL-Test VERSION 0.1
                    DESCRIPTION "OpenGL Test"
                    LANGUAGES C CXX)

# Include custom modules
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
include(utils)

prevent_in_src_build()

add_executable(main main.cpp)

set_target_properties(main PROPERTIES CXX_EXTENSIONS OFF)
target_compile_features(main PRIVATE cxx_std_17)
target_compile_options(main PRIVATE
    -Wall -Wextra -Wpedantic -Wconversion
    -Wno-unused-variable -Wno-unused-parameter
    #-g3
    #-Werror
)

add_subdirectory(stb_image)
target_include_directories(main PRIVATE stb_image)
target_link_libraries(main PRIVATE stb_image)

get_os_name(OS_)
get_linux_distro_name(DISTRO_)

if(${OS_} STREQUAL Linux)
    set(OpenGL_GL_PREFERENCE GLVND)
endif()

find_package(OpenGL REQUIRED)
target_link_libraries(main PRIVATE OpenGL::GL)

find_package(GLEW REQUIRED)
target_link_libraries(main PRIVATE GLEW::glew)

find_package(glfw3 3.3 REQUIRED)
target_link_libraries(main PRIVATE glfw)

find_package(glm REQUIRED)
if(${DISTRO_} STREQUAL arch)
    target_link_libraries(main PRIVATE glm)
else()
	target_link_libraries(main PRIVATE glm::glm)
endif()
