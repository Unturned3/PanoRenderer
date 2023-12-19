
cmake_minimum_required(VERSION 3.10)

include(utils.cmake)

project(OpenGL-Test)

prevent_in_src_build()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wpedantic -Wconversion")
set(WERROR off)

find_package(GLEW REQUIRED)
find_package(glfw3 3.3 REQUIRED)

add_executable(main main.cpp)
target_link_libraries(main GLEW::glew)
target_link_libraries(main glfw)

if(WERROR)
    append(CMAKE_CXX_FLAGS "-Werror")
endif()

