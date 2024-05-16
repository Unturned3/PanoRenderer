
#include <random>
#include <opencv2/opencv.hpp>
#include "Image.hpp"
#include "utils.hpp"
#include "AppState.hpp"

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

        n_frames = (int)cap.get(cv::CAP_PROP_FRAME_COUNT);
        std::random_device rd;
        std::mt19937 rng(rd());

        AppState &s = AppState::get();
        int want_frames = static_cast<int>(s.poses.value().shape[0]);

        std::uniform_int_distribution<int>uni(0, n_frames - want_frames);
        int random_integer = uni(rng);
        cap.set(cv::CAP_PROP_POS_FRAMES, random_integer);

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
