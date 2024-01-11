
add_library(imgui STATIC)

find_package(OpenGL REQUIRED)
target_link_libraries(imgui PRIVATE OpenGL::GL)

find_package(glfw3 3.3 REQUIRED)
target_link_libraries(imgui PRIVATE glfw)

target_include_directories(imgui PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/backends
    ${CMAKE_CURRENT_SOURCE_DIR}/misc/cpp
)

target_sources(imgui PRIVATE
    imgui.cpp
    imgui_demo.cpp
    imgui_draw.cpp
    imgui_widgets.cpp
    imgui_tables.cpp
    backends/imgui_impl_glfw.cpp
    backends/imgui_impl_opengl3.cpp
    misc/cpp/imgui_stdlib.cpp
)

target_compile_features(main PRIVATE cxx_std_11)
target_compile_options(main PRIVATE -Wall -Wformat -O3)
