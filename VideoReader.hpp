
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

enum class DecodePacketStatus {
    error,
    again,
    success,
};

class VideoReader {
public:
    VideoReader(std::string const &path)
    {
        pFormatContext = avformat_alloc_context();
        check(pFormatContext != nullptr, "Couldn't allocate AVFormat context");

        check(avformat_open_input(&pFormatContext, path.c_str(), nullptr,
                                  nullptr) == 0,
              fmt::format("Couldn't read file '{}'", path));

        check(avformat_find_stream_info(pFormatContext, nullptr) >= 0,
              "Couldn't get stream info");

        auto [streamIndex, pCodec, pCodecParameters] =
            get_video_stream_codec(pFormatContext);

        LOG("codec name, long_name: ", pCodec->name, ", ", pCodec->long_name);

        videoStreamIndex = streamIndex;
        width = pCodecParameters->width;
        height = pCodecParameters->height;

        pCodecContext = avcodec_alloc_context3(pCodec);
        check(pCodecContext, "Failed to allocate AVCodecContext");

        check(
            avcodec_parameters_to_context(pCodecContext, pCodecParameters) >= 0,
            "Failed to copy codec params to codec context");

        // Initialize the AVCodecContext to use the given AVCodec.
        check(avcodec_open2(pCodecContext, pCodec, nullptr) == 0,
              "Failed to open codec through avcodec_open2()");

        pFrame = av_frame_alloc();
        check(pFrame, "Failed to allocate AVFrame");

        pPacket = av_packet_alloc();
        check(pPacket, "Failed to allocate AVPacket");

        LOG("hwacel: ", pCodecContext->hwaccel_context != nullptr);
        LOG("h264_videotoolbox: ", avcodec_find_decoder_by_name("videotoolbox"));
    }

    ~VideoReader()
    {
        logging("Releasing all resources");
        avformat_close_input(&pFormatContext);
        av_packet_free(&pPacket);
        av_frame_free(&pFrame);
        avcodec_free_context(&pCodecContext);
    }

    VideoReader(const VideoReader &) = delete;
    VideoReader &operator=(const VideoReader &) = delete;

    VideoReader(VideoReader &&) = delete;
    VideoReader &operator=(VideoReader &&) = delete;

    uint8_t *readFrame()
    {
        while (true) {
            int ret = avcodec_receive_frame(pCodecContext, pFrame);
            if (ret == 0) {
                break;  // A new frame is returned
            }
            else if (ret == AVERROR(EAGAIN)) {
                // Need to send more packets before frames can be decoded
                if (av_read_frame(pFormatContext, pPacket) < 0) {
                    logging("av_read_frame(): EOF");
                    return nullptr;
                }
                if (pPacket->stream_index == videoStreamIndex) {
                    int r = avcodec_send_packet(pCodecContext, pPacket);
                    /* EAGAIN should never happen, since we call
                    avcodec_receive_frame prior to sending more frames. */
                    assert(r != AVERROR(EAGAIN));
                    if (r == AVERROR_EOF) {
                        av_packet_unref(pPacket);
                        return nullptr;
                    }
                    else if (r < 0) {
                        throw std::runtime_error(
                            fmt::format("Unknown error code {} from "
                                        "avcodec_send_packet()",
                                        r));
                    }
                }
                av_packet_unref(pPacket);
            }
            else if (ret == AVERROR_EOF) {
                logging("avcodec_receive_frame(): AVERROR_EOF");
                return nullptr;  // No more frames
            }
            else if (ret < 0) {
                // Decoding error
                logging("Error while receiving a frame from the decoder: %s",
                        av_err2str(ret));
                throw std::runtime_error("FFmpeg decoding failed");
            }
            else {
                throw std::runtime_error(
                    "Unknown return value from avcodec_receive_frame");
            }
        }
        assert(pFrame->linesize[0] == pFrame->width);
        assert(pFrame->width == width);
        assert(pFrame->height == height);
        return pFrame->data[0];
    }

private:
    AVFormatContext *pFormatContext;
    AVPacket *pPacket;
    AVFrame *pFrame;
    AVCodecContext *pCodecContext;
    int videoStreamIndex;

public:
    int width;
    int height;

    static std::tuple<int, AVCodec const *, AVCodecParameters *>
    get_video_stream_codec(AVFormatContext *pFormatContext)
    {
        AVCodec const *pCodec = nullptr;
        AVCodecParameters *pCodecParameters = nullptr;

        int stream_idx = -1;

        // Loop though all the streams to find the video stream
        for (int i = 0; i < static_cast<int>(pFormatContext->nb_streams); i++) {
            auto si = pFormatContext->streams[i];

            AVCodec const *pLocalCodec = nullptr;
            AVCodecParameters *pLocalCodecParameters = si->codecpar;

            logging("AVStream->time_base before open coded %d/%d",
                    si->time_base.num, si->time_base.den);
            logging("AVStream->r_frame_rate before open coded %d/%d",
                    si->r_frame_rate.num, si->r_frame_rate.den);
            logging("AVStream->start_time %" PRId64, si->start_time);
            logging("AVStream->duration %" PRId64, si->duration);

            logging("finding the proper decoder (CODEC)");

            // finds the registered decoder for a codec ID
            pLocalCodec = avcodec_find_decoder(pLocalCodecParameters->codec_id);

            if (pLocalCodec == nullptr) {
                logging("ERROR unsupported codec!");
                // In this example if the codec is not found we just skip it
                continue;
            }

            // when the stream is a video we store its index, codec parameters
            // and codec
            if (pLocalCodecParameters->codec_type == AVMEDIA_TYPE_VIDEO) {
                if (stream_idx == -1) {
                    stream_idx = i;
                    pCodec = pLocalCodec;
                    pCodecParameters = pLocalCodecParameters;
                }
                logging("Video Codec: resolution %d x %d",
                        pLocalCodecParameters->width,
                        pLocalCodecParameters->height);
            }

            // print its name, id and bitrate
            logging("\tCodec %s ID %d bit_rate %lld", pLocalCodec->name,
                    pLocalCodec->id, pLocalCodecParameters->bit_rate);
        }

        check(stream_idx != -1, "No video stream found!");
        logging("Video stream index: %d", stream_idx);

        return {stream_idx, pCodec, pCodecParameters};
    }
};
