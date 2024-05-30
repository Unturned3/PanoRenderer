
#pragma once

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <map>
#include <string>

#include "fmt/core.h"
#include "stb_image.h"
#include "stb_image_write.h"
#include "utils.hpp"

class Image;
std::map<Image const*, int> M;
int idx = 0;

class Image {
public:
    Image() : data_(nullptr), width_(0), height_(0), channels_(0)
    {
        if (!M.count(this)) M[this] = idx++;
        // LOG(M[this], " default constructor");
    }

    Image(std::string const& path, bool v_flip = true)
    {
        if (!M.count(this)) M[this] = idx++;
        // LOG(M[this], " path constructor");
        stbi_set_flip_vertically_on_load(v_flip);
        data_ = stbi_load(path.c_str(), &width_, &height_, &channels_, 0);
        if (!data_) throw std::runtime_error("Failed to load image " + path);
        if (channels_ != 3)
            throw std::runtime_error(
                fmt::format("Failed to load image {}. "
                            "Expected {} channels, but got {}.",
                            path, 3, channels_));
    }

    Image(int width, int height, int channels)
        : width_(width), height_(height), channels_(channels)
    {
        if (!M.count(this)) M[this] = idx++;
        // LOG(M[this], " alloc-only constructor");
        size_t sz = static_cast<size_t>(width_ * height_ * channels_);
        data_ = static_cast<uint8_t*>(malloc(sz));
        if (!data_) throw std::runtime_error("Failed to allocate memory.");
    }

    ~Image()
    {
        // LOG(M[this], " destructor");
        if (data_) {
            free(data_);
        }
    }

    friend void swap(Image& a, Image& b)
    {
        // LOG("swap image: ", M[&a], " ", M[&b]);
        //  https://stackoverflow.com/a/3279550/5702494
        using std::swap;
        swap(a.width_, b.width_);
        swap(a.height_, b.height_);
        swap(a.channels_, b.channels_);
        swap(a.data_, b.data_);
    }

    // Copy constructor
    Image(Image const& o) : Image(o.width_, o.height_, o.channels_)
    {
        if (!M.count(this)) M[this] = idx++;
        // LOG(M[this], " copy-constructing from ", M[&o]);
        size_t sz = static_cast<size_t>(width_ * height_ * channels_);
        memcpy(data_, o.data_, sz);
    }

    // Copy assignment operator
    Image& operator=(Image const& o)
    {
        if (!M.count(this)) M[this] = idx++;
        // LOG(M[this], " copy-assigning from ", M[&o]);
        /*  Apparently this is not good, and we should declare o as Image
            instead of Image const&. But, this prevents the compiler from
            confusing the copy-assignment with move-assignment operator.
        */
        Image tmp(o);
        swap(*this, tmp);
        return *this;
    }

    // Move constructor
    Image(Image&& o) : Image()
    {
        if (!M.count(this)) M[this] = idx++;
        // LOG(M[this], " move-constructing from ", M[&o]);
        swap(*this, o);
    }

    // Move assignment operator
    Image& operator=(Image&& o)
    {
        if (!M.count(this)) M[this] = idx++;
        // LOG(M[this], " move-assigning from ", M[&o]);
        swap(*this, o);
        return *this;
    }

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
