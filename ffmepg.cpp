
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
}
#include <fmt/core.h>
#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>

#include "Image.hpp"
#include "VideoReader.hpp"
#include "utils.hpp"

void save_gray_frame(unsigned char *buf, int wrap, int xsize, int ysize,
                     char *filename)
{
    logging("wrap: %d, xsize: %d, ysize: %d\n", wrap, xsize, ysize);
    // assert(wrap == xsize);
    Image f(xsize, ysize, 1);
    uint8_t *data = f.data();

    for (int i = 0; i < ysize; i++) {
        memcpy(data + i * xsize, buf + i * wrap, static_cast<size_t>(xsize));
    }
    // memcpy(f.data(), buf, xsize * ysize);

    int mn = 999, mx = -999;
    for (int i = 0; i < xsize * ysize; i++) {
        mn = std::min(mn, static_cast<int>(data[i]));
        mx = std::max(mx, static_cast<int>(data[i]));
    }
    logging("min/max values in frame: %d, %d", mn, mx);

    f.write(filename, false);
}

int main(int argc, const char *argv[])
{
    if (argc < 2) {
        std::cerr << "Usage: ffmpeg /path/to/video.mp4" << std::endl;
        return 1;
    }

    std::string path = argv[1];

    VideoReader v(path);
    Image f(v.width, v.height, 1);

    uint8_t *frame = nullptr;

    {
        utils::Timer<std::chrono::milliseconds> t;
        for (int i = 0; i < 999; i++) {
            frame = v.readFrame();
            //memcpy(f.data(), frame, static_cast<size_t>(v.width * v.height));
            //std::cout << f.data()[99999] << std::endl;
            // f.write(fmt::format("{}.jpg", i), false);
            if (!frame) {
                std::cout << "frame cnt: " << i << std::endl;
                break;
            }
        }
    }
    return 0;
}
