#ifndef SCREEN_RECORD_SRC_CAPTURER_PICTURE_CAPTURER_H_
#define SCREEN_RECORD_SRC_CAPTURER_PICTURE_CAPTURER_H_

#include <stdint.h>

#include <memory>

class PictureCapturer {
 public:
  class Picture {
   public:
    Picture();
    Picture(const Picture& other);
    Picture(Picture&& other);
    ~Picture();

    Picture& operator=(const Picture& other);
    Picture& operator=(Picture&&);

    int width() const { return width_; }
    int height() const { return height_; }
    int stride() const { return stride_; }
    uint8_t* data() const { return data_; }

    void Reset();
    void Reset(const uint8_t* data, int width, int height, int stride);

   private:
    int width_;
    int height_;
    int stride_;
    uint8_t* data_;
  };  // class Picture

  PictureCapturer() { }
  virtual ~PictureCapturer() { }

  virtual std::unique_ptr<Picture> CaptureScreen() = 0;
};  // class PictureCapturer

#endif  // SCREEN_RECORD_SRC_CAPTURER_PICTURE_CAPTURER_H_
