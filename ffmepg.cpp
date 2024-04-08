
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
#include "utils.hpp"

void logging(char const *format, ...)
{
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    printf("\n");
}

void save_gray_frame(unsigned char *buf, int wrap, int xsize, int ysize,
                     char *filename)
{
    logging("wrap: %d, xsize: %d, ysize: %d\n", wrap, xsize, ysize);
    //assert(wrap == xsize);
    Image f(xsize, ysize, 1);
    uint8_t *data = f.data();

    for (int i = 0; i < ysize; i++) {
        memcpy(data + i * xsize, buf + i * wrap, xsize);
    }
    //memcpy(f.data(), buf, xsize * ysize);

    int mn = 999, mx = -999;
    for (int i = 0; i < xsize * ysize; i++) {
        mn = std::min(mn, static_cast<int>(data[i]));
        mx = std::max(mx, static_cast<int>(data[i]));
    }
    logging("min/max values in frame: %d, %d", mn, mx);

    f.write(filename, false);
}

int decode_packet(AVPacket *pPacket, AVCodecContext *pCodecContext,
                  AVFrame *pFrame)
{
    int response = avcodec_send_packet(pCodecContext, pPacket);

    if (response < 0) {
        logging("Error while sending a packet to the decoder: %s",
                av_err2str(response));
        return response;
    }

    while (response >= 0) {
        // Return decoded output data (into a frame) from a decoder
        // https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga11e6542c4e66d3028668788a1a74217c
        response = avcodec_receive_frame(pCodecContext, pFrame);
        if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
            break;
        }
        else if (response < 0) {
            logging("Error while receiving a frame from the decoder: %s",
                    av_err2str(response));
            return response;
        }

        if (response >= 0) {
            logging(
                "Frame %d (type=%c, size=%d bytes, format=%d) pts %d key_frame "
                "%d [DTS %d]",
                pCodecContext->frame_number,
                av_get_picture_type_char(pFrame->pict_type), pFrame->pkt_size,
                pFrame->format, pFrame->pts, pFrame->key_frame,
                pFrame->coded_picture_number);

            char frame_filename[1024];
            snprintf(frame_filename, sizeof(frame_filename), "%s-%d.jpg",
                     "frame", pCodecContext->frame_number);
            // Check if the frame is a planar YUV 4:2:0, 12bpp
            // That is the format of the provided .mp4 file
            // RGB formats will definitely not give a gray image
            // Other YUV image may do so, but untested, so give a warning
            if (pFrame->format != AV_PIX_FMT_YUV420P) {
                logging(
                    "Warning: the generated file may not be a grayscale image, "
                    "but could e.g. be just the R component if the video "
                    "format is RGB");
                logging("Got format: %d", pFrame->format);
            }
            // save a grayscale frame into a .pgm file
            save_gray_frame(pFrame->data[0], pFrame->linesize[0], pFrame->width,
                            pFrame->height, frame_filename);
        }
    }
    return 0;
}

int main(int argc, const char *argv[])
{
    if (argc < 2) {
        std::cerr << "Usage: ffmpeg /path/to/video.mp4" << std::endl;
        return 1;
    }

    std::string inputFilePath = argv[1];

    /*
    auto formatContext =
        utils::make_unique(avformat_alloc_context(), avformat_free_context);
    auto k = formatContext.get();
    */

    AVFormatContext *pFormatContext = avformat_alloc_context();
    check(pFormatContext != nullptr, "Couldn't allocate AVFormat context");

    check(avformat_open_input(&pFormatContext, argv[1], nullptr, nullptr) == 0,
          fmt::format("Couldn't read file '{}'", inputFilePath));

    check(avformat_find_stream_info(pFormatContext, NULL) >= 0,
          "Couldn't get stream info");

    AVCodec const *pCodec = NULL;
    AVCodecParameters *pCodecParameters = NULL;

    int video_stream_index = -1;

    // Loop though all the streams to find the video stream
    for (int i = 0; i < static_cast<int>(pFormatContext->nb_streams); i++) {
        auto si = pFormatContext->streams[i];
        AVCodecParameters *pLocalCodecParameters = si->codecpar;

        logging("AVStream->time_base before open coded %d/%d",
                si->time_base.num, si->time_base.den);
        logging("AVStream->r_frame_rate before open coded %d/%d",
                si->r_frame_rate.num, si->r_frame_rate.den);
        logging("AVStream->start_time %" PRId64, si->start_time);
        logging("AVStream->duration %" PRId64, si->duration);

        logging("finding the proper decoder (CODEC)");

        AVCodec const *pLocalCodec = NULL;

        // finds the registered decoder for a codec ID
        pLocalCodec = avcodec_find_decoder(pLocalCodecParameters->codec_id);

        if (pLocalCodec == NULL) {
            logging("ERROR unsupported codec!");
            // In this example if the codec is not found we just skip it
            continue;
        }

        // when the stream is a video we store its index, codec parameters and
        // codec
        if (pLocalCodecParameters->codec_type == AVMEDIA_TYPE_VIDEO) {
            if (video_stream_index == -1) {
                video_stream_index = i;
                pCodec = pLocalCodec;
                pCodecParameters = pLocalCodecParameters;
            }
            else if (true) {
            }

            logging("Video Codec: resolution %d x %d",
                    pLocalCodecParameters->width,
                    pLocalCodecParameters->height);
        }
        else if (pLocalCodecParameters->codec_type == AVMEDIA_TYPE_AUDIO) {
            logging("Audio Codec: %d channels, sample rate %d",
                    pLocalCodecParameters->channels,
                    pLocalCodecParameters->sample_rate);
        }

        // print its name, id and bitrate
        logging("\tCodec %s ID %d bit_rate %lld", pLocalCodec->name,
                pLocalCodec->id, pLocalCodecParameters->bit_rate);
    }

    check(video_stream_index != -1, "No video stream found!");
    logging("Video stream index: %d", video_stream_index);

    AVCodecContext *pCodecContext = avcodec_alloc_context3(pCodec);
    check(pCodecContext, "Failed to allocate AVCodecContext");

    check(avcodec_parameters_to_context(pCodecContext, pCodecParameters) >= 0,
          "Failed to copy codec params to codec context");

    // Initialize the AVCodecContext to use the given AVCodec.
    check(avcodec_open2(pCodecContext, pCodec, NULL) == 0,
          "Failed to open codec through avcodec_open2()");

    AVFrame *pFrame = av_frame_alloc();
    check(pFrame, "Failed to allocate AVFrame");

    AVPacket *pPacket = av_packet_alloc();
    check(pPacket, "Failed to allocate AVPacket");

    int response = 0;
    int pkts_to_process = 4;

    // Fill the packet with data from the Stream
    // Bad naming... av_read_frame ac
    while (av_read_frame(pFormatContext, pPacket) >= 0) {
        // If it's the video stream
        if (pPacket->stream_index == video_stream_index) {
            logging("AVPacket->pts %" PRId64, pPacket->pts);
            response = decode_packet(pPacket, pCodecContext, pFrame);
            if (response < 0) break;
            if (--pkts_to_process <= 0) break;
        }
        av_packet_unref(pPacket);
    }

    logging("Releasing all resources");

    avformat_close_input(&pFormatContext);
    av_packet_free(&pPacket);
    av_frame_free(&pFrame);
    avcodec_free_context(&pCodecContext);

    return 0;
}
