
#include <opencv2/opencv.hpp>
#include <random>
#include "AppState.hpp"
#include "Image.hpp"
#include "utils.hpp"

class PanoContainer {
public:
    PanoContainer() = default;

    PanoContainer(Image img) : img(img)
    {
        isVideo = false;
        width = img.width();
        height = img.height();
        data = this->img.data();
    }

    PanoContainer(cv::VideoCapture cap) : cap(cap)
    {
        check(this->cap.isOpened(), "Error opening video file");
        isVideo = true;
        width = (int)cap.get(cv::CAP_PROP_FRAME_WIDTH);
        height = (int)cap.get(cv::CAP_PROP_FRAME_HEIGHT);

        /*  NOTE: Sometimes OpenCV fails to read the last few frames of a
            video. We just avoid the last 30 frames to be safe. */
        int total_frames = (int)cap.get(cv::CAP_PROP_FRAME_COUNT) - 30;

        AppState &s = AppState::get();
        int want_frames = static_cast<int>(s.poses.value().shape[0]);
        int start_frame = s.start_frame;

        if (start_frame + want_frames > total_frames) {
            throw std::runtime_error(fmt::format(
                "Error: requested {} frames starting at "
                "frame {}, but only {} frames are available.",
                want_frames, start_frame, total_frames));
        }

        cap.set(cv::CAP_PROP_POS_FRAMES, start_frame);
        nextFrame();
    }

    void nextFrame()
    {
        this->cap >> frame;
        this->data = frame.data;
    }

public:
    bool isVideo;
    Image img;
    cv::VideoCapture cap;
    int width, height;
    cv::Mat frame;
    uint8_t *data;
    int n_frames;
};
