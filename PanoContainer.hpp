
#include <opencv2/opencv.hpp>
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
};
