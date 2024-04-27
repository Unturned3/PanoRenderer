
add_library(cnpy STATIC)

target_include_directories(cnpy PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_sources(cnpy PRIVATE
	cnpy.cpp
)

target_compile_options(cnpy PRIVATE -O3 -w)

