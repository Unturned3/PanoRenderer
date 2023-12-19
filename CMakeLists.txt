
cmake_minimum_required(VERSION 3.10)

include(utils.cmake)

project(OpenGL-Test)

prevent_in_src_build()

set(OS_ ${CMAKE_SYSTEM_NAME})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wpedantic -Wconversion")
append(CMAKE_CXX_FLAGS "-Wno-unused-variable -Wno-unused-parameter")
set(WERROR off)

add_executable(main main.cpp)

#if(${OS_} STREQUAL Linux)
    set(OpenGL_GL_PREFERENCE GLVND)
    find_package(OpenGL REQUIRED)
    target_link_libraries(main OpenGL::GL)
#endif()

find_package(GLEW REQUIRED)
target_link_libraries(main GLEW::glew)

find_package(glfw3 3.3 REQUIRED)
target_link_libraries(main glfw)

if(WERROR)
    append(CMAKE_CXX_FLAGS "-Werror")
endif()

