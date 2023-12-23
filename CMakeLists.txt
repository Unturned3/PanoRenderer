
cmake_minimum_required(VERSION 3.10)

include(utils.cmake)

project(OpenGL-Test)

prevent_in_src_build()

set(OS_ ${CMAKE_SYSTEM_NAME})
execute_process(
        COMMAND bash -c
        "cat /etc/os-arelease 2> /dev/null | grep '^ID=' | cut -d'=' -f 2"
        OUTPUT_VARIABLE DISTRO_)
if("${DISTRO_}" STREQUAL "")
    set(DISTRO_ n/a)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wpedantic -Wconversion")
append(CMAKE_CXX_FLAGS "-Wno-unused-variable -Wno-unused-parameter")
set(WERROR off)

add_executable(main main.cpp stb_image.c stb_image_write.c)

if(${OS_} STREQUAL Linux)
    set(OpenGL_GL_PREFERENCE GLVND)
endif()
find_package(OpenGL REQUIRED)
target_link_libraries(main OpenGL::GL)

find_package(GLEW REQUIRED)
target_link_libraries(main GLEW::glew)

find_package(glfw3 3.3 REQUIRED)
target_link_libraries(main glfw)

find_package(glm REQUIRED)
if((${OS_} STREQUAL Linux) AND (${DISTRO_} STREQUAL arch))
    target_link_libraries(main glm)
else()
	target_link_libraries(main glm::glm)
endif()

if(WERROR)
    append(CMAKE_CXX_FLAGS "-Werror")
endif()

