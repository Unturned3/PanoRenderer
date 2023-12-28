
add_library(stb_image STATIC)

target_sources(stb_image PRIVATE
    stb_image.h
    stb_image_write.h
    stb_image.c
    stb_image_write.c
)

target_compile_options(stb_image PRIVATE -O3)
