#include "screen_record/src/capturer/picture_capturer.h"

#include <QtCore/QDebug>

// PictureCapturer::Picture ----------------------------------------------------

PictureCapturer::Picture::Picture()
    : width_(0), height_(0), stride_(0), data_(nullptr) {}

PictureCapturer::Picture::Picture(const Picture& other)
    : width_(other.width_), height_(other.height_), stride_(other.stride_) {
  if (other.data_) {
    int len = stride_ * height_;
    Q_ASSERT(len > 0);

    data_ = new uint8_t[len];
    memcpy(data_, other.data_, sizeof(uint8_t) * len);
  }
}

PictureCapturer::Picture::Picture(Picture&& other)
    : width_(other.width_),
      height_(other.height_),
      stride_(other.stride_),
      data_(other.data_) {
  other.width_ = other.height_ = other.stride_ = 0;
  other.data_ = nullptr;
}

PictureCapturer::Picture::~Picture() {
  width_ = height_ = stride_ = 0;
  if (data_) {
    delete[] data_;
    data_ = nullptr;
  }
}

void PictureCapturer::Picture::Reset() {
  width_ = height_ = stride_ = 0;
  if (data_) {
    delete[] data_;
    data_ = nullptr;
  }
}

void PictureCapturer::Picture::Reset(const uint8_t* data,
                                     int width,
                                     int height,
                                     int stride) {
  Q_ASSERT(width > 0 && height > 0 && stride > 0 && data);

  if (data_) {
    delete[] data_;
  }

  width_ = width;
  height_ = height;
  stride_ = stride;

  int len = stride * height;
  data_ = new uint8_t[len];
  memcpy(data_, data, sizeof(uint8_t) * len);
}

PictureCapturer::Picture& PictureCapturer::Picture::operator=(
    const Picture& other) {
  width_ = other.width_;
  height_ = other.height_;
  stride_ = other.stride_;

  if (other.data_) {
    Q_ASSERT(width_ > 0 && stride_ > 0 && height_ > 0);
    int len = stride_ * height_;

    data_ = new uint8_t[len];
    memcpy(data_, other.data_, sizeof(uint8_t) * len);
  }

  return *this;
}

PictureCapturer::Picture& PictureCapturer::Picture::operator=(Picture&& other) {
  width_ = other.width_;
  height_ = other.height_;
  stride_ = other.stride_;
  data_ = other.data_;

  other.width_ = other.height_ = other.stride_ = 0;
  other.data_ = nullptr;

  return *this;
}
