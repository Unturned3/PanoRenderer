
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
#include <string>

#include <opencv2/opencv.hpp>

#include "VideoReader.hpp"
#include "utils.hpp"

int test(int argc, const char *argv[])
{
    AVFormatContext *input_ctx = NULL;
    int video_stream, ret;
    AVStream *video = NULL;
    AVCodecContext *decoder_ctx = NULL;
    const AVCodec *decoder = NULL;
    AVPacket *packet = NULL;
    enum AVHWDeviceType type;

    type = av_hwdevice_find_type_by_name("videotoolbox");
    check(type != AV_HWDEVICE_TYPE_NONE, "No hwaccel found");
    return 0;
}

int main(int argc, const char *argv[])
{
    if (argc < 2) {
        std::cerr << "Usage: ffmpeg /path/to/video.mp4" << std::endl;
        return 1;
    }

    std::string path = argv[1];

#define TEST_OPENCV

#ifdef TEST_OPENCV
    cv::VideoCapture cap(path, cv::CAP_FFMPEG);
    check(cap.isOpened(), "Error opening video file");

    int fourcc = cv::VideoWriter::fourcc('a', 'v', 'c', '1');
    cv::VideoWriter writer("out.mp4", fourcc, 30, {1024, 512}, true);
    check(writer.isOpened(), "Error opening cv::VideoWriter");

    {
        utils::Timer<std::chrono::milliseconds> t("Total decode time: ", "ms");
        while (true) {
            cv::Mat frame;
            cap >> frame;
            if (frame.empty())
                break;
            cv::resize(frame, frame, {1024, 512});
            writer.write(frame);
        }
    }
    cap.release();
    writer.release();
    cv::destroyAllWindows();
#else
    VideoReader v(path);
    Image f(v.width, v.height, 1);

    uint8_t *frame = nullptr;

    {
        utils::Timer<std::chrono::milliseconds> t("ms");
        for (int i = 0; i < 999; i++) {
            frame = v.readFrame();
            if (!frame) {
                std::cout << "frame cnt: " << i << std::endl;
                break;
            }
            memcpy(f.data(), frame, static_cast<size_t>(v.width * v.height));
            //std::cout << f.data()[99999] << std::endl;
            //f.write(fmt::format("{}.jpg", i), false);
        }
    }
#endif
    return 0;
}
