
#pragma once

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <format>
#include <string>

#include "stb_image.h"
#include "stb_image_write.h"

class Image {
public:
    Image(std::string const& path, bool v_flip = true)
    {
        stbi_set_flip_vertically_on_load(v_flip);
        data_ = stbi_load(path.c_str(), &width_, &height_, &channels_, 0);
        if (!data_) throw std::runtime_error("Failed to load image " + path);
        if (channels_ != 3)
            throw std::runtime_error(
                std::format("Failed to load image {}. "
                            "Expected {} channels, but got {}.",
                            path, 3, channels_));
    }

    Image(int width, int height, int channels)
        : width_(width), height_(height), channels_(channels)
    {
        size_t sz = static_cast<size_t>(width_ * height_ * channels_);
        data_ = static_cast<uint8_t*>(malloc(sz));
        if (!data_) throw std::runtime_error("Failed to allocate memory.");
    }

    ~Image() { free(data_); }

    Image(Image const& o)
    {
        width_ = o.width_;
        height_ = o.height_;
        channels_ = o.channels_;
        size_t sz = static_cast<size_t>(width_ * height_ * channels_);
        data_ = static_cast<uint8_t*>(malloc(sz));
        if (!data_) throw std::runtime_error("Failed to allocate memory.");
        memcpy(data_, o.data_, sz);
    }

    Image& operator=(Image const& o) = delete;

    Image(Image&& o) = delete;

    Image& operator=(Image&& o) = delete;

    void write(std::string const& name, bool v_flip = true, int quality = 90)
    {
        stbi_flip_vertically_on_write(v_flip);
        stbi_write_jpg(name.c_str(), width_, height_, channels_, data_,
                       quality);
    }

    uint8_t* data() { return data_; }

    int width() { return width_; }

    int height() { return height_; }

    int channels() { return channels_; }

    int size() { return width_ * height_ * channels_; }

private:
    uint8_t* data_;
    int width_, height_, channels_;
};
