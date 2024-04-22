
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavcodec/videotoolbox.h>
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

        AVCodec const *pCodec = nullptr;
        videoStreamIndex = av_find_best_stream(
            pFormatContext, AVMEDIA_TYPE_VIDEO, -1, -1, &pCodec, 0);
        check(videoStreamIndex >= 0, "No video stream found");

        AVStream *video = pFormatContext->streams[videoStreamIndex];
        width = video->codecpar->width;
        height = video->codecpar->height;

        hwDevType = av_hwdevice_find_type_by_name("videotoolbox");
        check(hwDevType != AV_HWDEVICE_TYPE_NONE, "No hwaccel found");

        utils::log("Listing available AVCodecHWConfig in pCodec:");

        for (int i = 0;; i++) {
            AVCodecHWConfig const *c = avcodec_get_hw_config(pCodec, i);
            if (!c) break;
            logging("%d, pix_fmt: %d", i, c->pix_fmt);
        }

        for (int i = 0;; i++) {
            AVCodecHWConfig const *c = avcodec_get_hw_config(pCodec, i);
            check(c, fmt::format("Decoder {} doesn't support {}", pCodec->name,
                                 av_hwdevice_get_type_name(hwDevType), 'a'));
            if (c->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX &&
                c->device_type == hwDevType) {
                hwPixFmt = c->pix_fmt;
                break;
            }
        }

        logging("Found hwPixFmt: %d", hwPixFmt);

        pCodecContext = avcodec_alloc_context3(pCodec);
        check(pCodecContext, "Failed to allocate AVCodecContext");

        check(
            avcodec_parameters_to_context(pCodecContext, video->codecpar) >= 0,
            "Failed to copy codec params to codec context");

        pCodecContext->get_format = get_hw_format;
        check(hw_decoder_init(pCodecContext, hwDevType) >= 0,
              "hw_decoder_init() failed");

        // Initialize the AVCodecContext to use the given AVCodec.
        check(avcodec_open2(pCodecContext, pCodec, nullptr) == 0,
              "Failed to open codec through avcodec_open2()");

        pFrame = av_frame_alloc();
        check(pFrame, "Failed to allocate AVFrame");

        pPacket = av_packet_alloc();
        check(pPacket, "Failed to allocate AVPacket");

        logging(
            "pCodecContext: hwaccel 0x%lx, hwaccel_context 0x%lx, "
            "hw_device_ctx 0x%lx",
            pCodecContext->hwaccel, pCodecContext->hwaccel_context,
            pCodecContext->hw_device_ctx);
    }

    ~VideoReader()
    {
        logging("Releasing all resources");
        avformat_close_input(&pFormatContext);
        av_packet_free(&pPacket);
        av_frame_free(&pFrame);
        avcodec_free_context(&pCodecContext);
        av_buffer_unref(&hwDevContext);
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
                /*
                logging("Error while receiving a frame from the decoder: %s",
                        av_err2str(ret));
                        */
                logging("Error while receiving a frame from the decoder");
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

    AVHWDeviceType hwDevType;
    AVPixelFormat hwPixFmt;
    AVBufferRef *hwDevContext;

    static AVPixelFormat get_hw_format(AVCodecContext *ctx,
                                       const AVPixelFormat *pix_fmts)
    {
        //return AV_PIX_FMT_YUVJ420P;

        AVPixelFormat const *fmt;

        int i = 0;
        for (fmt = pix_fmts; *fmt != AV_PIX_FMT_NONE; fmt++) {
            // See:
            // https://medium.com/liveop-x-team/accelerating-h264-
            // decoding-on-ios-with-ffmpeg-and-videotoolbox-1f000cb6c549
            if (*fmt == AV_PIX_FMT_VIDEOTOOLBOX) {
                if (ctx->hwaccel_context == nullptr) {
                    int r = av_videotoolbox_default_init(ctx);
                    if (r < 0) {
                        logging("r < 0. ctx->pix_fmt: %d", ctx->pix_fmt);
                        return ctx->pix_fmt;
                    }
                }
                logging("Returning fmt: %d", *fmt);
                return *fmt;
            }
            logging("%d, get_hw_format skipping fmt: %d", i, *fmt);
            i++;
        }

        LOG("Failed to get HW surface format.");
        return AV_PIX_FMT_NONE;
    }

    int hw_decoder_init(AVCodecContext *ctx, const enum AVHWDeviceType type)
    {
        int err = 0;

        hwDevContext = nullptr;

        if ((err = av_hwdevice_ctx_create(&hwDevContext, type, NULL, NULL, 0)) <
            0) {
            fprintf(stderr, "Failed to create specified HW device.\n");
            return err;
        }
        ctx->hw_device_ctx = av_buffer_ref(hwDevContext);
        logging("hwDevContext: 0x%lx", hwDevContext);
        logging("hw_decoder_init ctx->hw_device_ctx: 0x%lx",
                ctx->hw_device_ctx);

        return err;
    }

public:
    int width;
    int height;
};
